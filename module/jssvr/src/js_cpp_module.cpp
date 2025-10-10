#include "js_cpp_module.h"

#include <shared_mutex>

#include <million/config/ss_config.pb.h>
#include <million/jssvr/ss_jssvr.pb.h>
#include <million/jssvr/jssvr.h>

#include "js_service.h"
#include "js_util.h"

namespace million {
namespace jssvr {

MillionModuleObject::MillionModuleObject(mjs::Runtime* rt)
    : CppModuleObject(rt)
{
    AddExportMethod(rt, "newservice", NewService);
    AddExportMethod(rt, "makemsg", MakeMessage);
    AddExportMethod(rt, "timeout", Timeout);
}

mjs::Value MillionModuleObject::NewService(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSRuntineService(&context->runtime());
    if (par_count < 1) {
        return mjs::Error::Throw(context, "Creating a service requires 1 parameter.");
    }
    NewJSService(&service.imillion(), stack.get(0).ToString(context).string_view());
    return mjs::Value();
}

mjs::Value MillionModuleObject::MakeMessage(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    if (par_count < 2) {
        return mjs::Error::Throw(context, "MakeMessage requires 2 parameter.");
    }
    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "MakeMessage parameter 1 must be a string for creation.");
    }
    if (!stack.get(1).IsObject()) {
        return mjs::Error::Throw(context, "MakeMessage parameter 2 must be a object for creation.");
    }

    auto array = mjs::ArrayObject::New(context, 2);
    array->operator[](0) = std::move(stack.get(0));
    array->operator[](1) = std::move(stack.get(1));

    return mjs::Value(array);
}

mjs::Value MillionModuleObject::Timeout(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    if (par_count < 2) {
        return mjs::Error::Throw(context, "Timeout requires 2 parameter.");
    }
    if (!stack.get(0).IsNumber()) {
        return mjs::Error::Throw(context, "Timeout parameter 1 must be a number for creation.");
    }
    if (!stack.get(1).IsObject()) {
        return mjs::Error::Throw(context, "Timeout parameter 2 must be a object for creation.");
    }

    auto& service = GetJSRuntineService(&context->runtime());

    auto predicate_func = stack.get(0);

    service.imillion().Timeout(stack.get(0).ToUInt64().u64(), service.service_handle(), make_message<JsServiceTimeoutMessage>(stack.get(0).ToUInt64().u64(), predicate_func));

    return mjs::Value();
}


ServiceModuleObject::ServiceModuleObject(mjs::Runtime* rt)
    : CppModuleObject(rt)
{
    AddExportMethod(rt, "send", Send);
    AddExportMethod(rt, "call", Call);
    AddExportMethod(rt, "reply", Reply);
}

mjs::Value ServiceModuleObject::Send(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto result = Call(context, par_count, stack);
    auto& service = GetJSService(context);
    service.function_call_context().waiting_session_id.reset();
    return result;
}

mjs::Value ServiceModuleObject::Call(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSService(context);
    if (par_count < 4) {
        return mjs::Error::Throw(context, "Send requires 4 parameter.");
    }

    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "The service name is not a string.");
    }
    if (!stack.get(1).IsString()) {
        return mjs::Error::Throw(context, "The service name is not a string.");
    }
    if (!stack.get(2).IsString()) {
        return mjs::Error::Throw(context, "The message name is not a string.");
    }
    if (!stack.get(3).IsObject()) {
        return mjs::Error::Throw(context, "The message is not an object.");
    }


    uint32_t module_id = service.imillion().proto_mgr().FindEnumValueByFullName(stack.get(0).string_view());
    if (module_id == 0) {
        return mjs::Error::Throw(context, "Invalid module id full name: {}", stack.get(0).string_view());
    }

    uint32_t service_name_id = service.imillion().proto_mgr().FindEnumValueByFullName(stack.get(1).string_view());
    if (service_name_id == 0) {
        return mjs::Error::Throw(context, "Invalid service id full name: {}", stack.get(1).string_view());
    }

    auto target = service.imillion().FindServiceByNameId(million::EncodeModuleCode(module_id, service_name_id));
    if (!target) {
        return mjs::Value();
    }

    auto desc = service.imillion().proto_mgr().FindMessageTypeByName(stack.get(2).string_view());
    if (!desc) {
        return mjs::Value();
    }
    auto msg = service.imillion().proto_mgr().NewMessage(*desc);

    JSUtil::JSObjectToProtoMessage(context, msg.get(), stack.get(3));

    service.function_call_context().waiting_session_id = service.Send(*target, MessagePointer(std::move(msg)));
    service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(context, mjs::Value()));
    return service.function_call_context().promise;
}

mjs::Value ServiceModuleObject::Reply(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSService(context);
    if (par_count < 5) {
        return mjs::Error::Throw(context, "Send requires 5 parameter.");
    }

    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "The service name is not a string.");
    }
    if (!stack.get(1).IsString()) {
        return mjs::Error::Throw(context, "The service name is not a string.");
    }
    if (!stack.get(2).IsNumber()) {
        return mjs::Error::Throw(context, "The message name is not a number.");
    }
    if (!stack.get(3).IsString()) {
        return mjs::Error::Throw(context, "The message name is not a string.");
    }
    if (!stack.get(4).IsObject()) {
        return mjs::Error::Throw(context, "The message is not an object.");
    }


    uint32_t module_id = service.imillion().proto_mgr().FindEnumValueByFullName(stack.get(0).string_view());
    if (module_id == 0) {
        return mjs::Error::Throw(context, "Invalid module id full name: {}", stack.get(0).string_view());
    }

    uint32_t service_name_id = service.imillion().proto_mgr().FindEnumValueByFullName(stack.get(1).string_view());
    if (service_name_id == 0) {
        return mjs::Error::Throw(context, "Invalid service id full name: {}", stack.get(1).string_view());
    }

    uint64_t agent_id = stack.get(2).ToUInt64().u64();
    if (agent_id == 0) {
        return mjs::Error::Throw(context, "Invalid agent_id: {}", agent_id);
    }


    auto target = service.imillion().FindServiceByNameId(module_id);
    if (!target) {
        return mjs::Value();
    }

    auto desc = service.imillion().proto_mgr().FindMessageTypeByName(stack.get(3).string_view());
    if (!desc) {
        return mjs::Value();
    }
    auto msg = service.imillion().proto_mgr().NewMessage(*desc);

    JSUtil::JSObjectToProtoMessage(context, msg.get(), stack.get(4));

   service.Reply(*target, agent_id, MessagePointer(std::move(msg)));

    return mjs::Value();
}


LoggerModuleObject::LoggerModuleObject(mjs::Runtime* rt)
    : CppModuleObject(rt)
{
    AddExportMethod(rt, "debug", Debug);
    AddExportMethod(rt, "info", Info);
    AddExportMethod(rt, "error", Error);
}

mjs::Value LoggerModuleObject::Debug(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSRuntineService(&context->runtime());
    auto source_location = GetSourceLocation(stack);
    for (uint32_t i = 0; i < par_count; ++i) {
        service.logger().Log(source_location, ::million::Logger::LogLevel::kDebug, stack.get(i).ToString(context).string_view());
    }
    return mjs::Value();
}

mjs::Value LoggerModuleObject::Info(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSRuntineService(&context->runtime());
    auto source_location = GetSourceLocation(stack);
    for (uint32_t i = 0; i < par_count; ++i) {
        service.logger().Log(source_location, ::million::Logger::LogLevel::kInfo, stack.get(i).ToString(context).string_view());
    }
    return mjs::Value();
}

mjs::Value LoggerModuleObject::Error(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSRuntineService(&context->runtime());
    auto source_location = GetSourceLocation(stack);
    for (uint32_t i = 0; i < par_count; ++i) {
        service.logger().Log(source_location, ::million::Logger::LogLevel::kError, stack.get(i).ToString(context).string_view());
    }
    return mjs::Value();
}

million::SourceLocation LoggerModuleObject::GetSourceLocation(const mjs::StackFrame& stack) {
    const mjs::StackFrame* js_stack = &stack;
    while (!js_stack->function_def()) {
        if (!js_stack->upper_stack_frame()) {
            break;
        }
        js_stack = js_stack->upper_stack_frame();
    }
    auto debug_info = js_stack->function_def()->debug_table().FindEntry(js_stack->pc());
    if (debug_info && js_stack) {
        auto source_location = million::SourceLocation{
            .line = debug_info->source_line,
            .column = 0,
            .file_name = js_stack->function_def()->module_def().name(),
            .function_name = js_stack->function_def()->name(),
        };
        return source_location;
    }
    else {
        auto source_location = million::SourceLocation{
            .line = 0,
            .column = 0,
            .file_name = "<unknown>",
            .function_name = "<unknown>",
        };
        return source_location;
    }
}


DBModuleObject::DBModuleObject(mjs::Runtime* rt) :
    CppModuleObject(rt)
{
    AddExportMethod(rt, "load", Load);
}

mjs::Value DBModuleObject::Load(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSService(context);

    if (par_count < 1) {
        return mjs::Error::Throw(context, "DB Load requires 1 parameter.");
    }

    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "DB Load parameter 1 must be a string for message type.");
    }
    if (!stack.get(1).IsString()) {
        return mjs::Error::Throw(context, "DB Load parameter 2 must be a string for key field name.");
    }

    auto msg_name = stack.get(0).string_view();

    const auto* desc = service.imillion().proto_mgr().FindMessageTypeByName(msg_name);
    if (!desc) {
        return mjs::Error::Throw(context, "DB Load parameter 1 invalid message type.");
    }

    auto key_field_name = stack.get(1).string_view();
    const auto* field = desc->FindFieldByName(key_field_name);
    if (!field) {
        return mjs::Error::Throw(context, "DB Load parameter 2 invalid field name.");
    }

    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "DB Load parameter must be a string for file name.");
    }

    auto key = stack.get(2).ToString(context);
    if (key.IsException()) {
        return mjs::Error::Throw(context, "DB Load parameter 3 must be a string for key value.");
    }
    
    // 这里让OnMsg等待，发现是C++消息再做分发

    service.function_call_context().sender = service.js_runtime_service().db_service_handle();

    service.function_call_context().waiting_session_id = service.Send<db::DBRowLoadReq>(service.function_call_context().sender, *desc
        , field->number(), std::string(key.string_view()));

    service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(context, mjs::Value()));
    return service.function_call_context().promise;
}



DBRowClassDef::DBRowClassDef(mjs::Runtime* runtime) 
    : ClassDef(runtime, static_cast<mjs::ClassId>(CustomClassId::kDBRowObject), "DBRow") {

    auto const_index = runtime->const_pool().insert(mjs::Value("commit"));

    prototype_.object().SetProperty(nullptr, const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const  mjs::StackFrame& stack) -> mjs::Value {
        auto& service = GetJSService(context);

        service.function_call_context().sender = service.js_runtime_service().db_service_handle();

        auto& db_row_object = stack.this_val().object<DBRowObject>();

        auto task = db_row_object.db_row().Commit(&service, service.function_call_context().sender);
        service.function_call_context().waiting_session_id = task.coroutine.promise().session_awaiter()->waiting_session_id();

        // Start async commit operation
        service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(context, mjs::Value()));

        return service.function_call_context().promise;
    }));
}


DBRowObject::DBRowObject(mjs::Context* context, db::DBRow&& db_row)
    : Object(context, static_cast<mjs::ClassId>(CustomClassId::kDBRowObject))
    , db_row_(std::move(db_row)) {}


void DBRowObject::SetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value&& value) {
    auto key_str = context->GetConstValue(key).string_view();
    
    const auto& desc = db_row_.GetDescriptor();
    const auto& reflection = db_row_.GetReflection();
    
    // Find the field by name
    const auto* field = desc.FindFieldByName(key_str);
    if (!field) {
        return; // Field not found, ignore silently like JavaScript objects
    }
    
    // Mark field as dirty when setting
    db_row_.MarkDirtyByFieldIndex(field->index());
    
    // Convert JavaScript value to protobuf field
    auto& proto_msg = db_row_.get();
    JSUtil::SetProtoMessageFieldFromJSValue(context, &proto_msg, desc, reflection, *field, value);
}

bool DBRowObject::GetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value* value) {
    auto key_str = context->GetConstValue(key).string_view();
    
    const auto& desc = db_row_.GetDescriptor();
    const auto& reflection = db_row_.GetReflection();
    
    // Find the field by name
    const auto* field = desc.FindFieldByName(key_str);
    if (!field) {
        return Object::GetProperty(context, key, value); // Field not found
    }
    
    // Convert protobuf field to JavaScript value
    const auto& proto_msg = db_row_.get();
    *value = JSUtil::GetJSValueByProtoMessageField(context, proto_msg, reflection, *field);
    return true;
}




// JSConfigService implementation
JSConfigService::JSConfigService(IMillion* imillion, JSRuntimeService* js_runtime_service)
    : Base(imillion)
    , js_runtime_service_(js_runtime_service) {
}

JSConfigService::~JSConfigService() = default;

bool JSConfigService::OnInit() {
    // 获取配置服务句柄
    auto config_service_opt = imillion().FindServiceByNameId(module::module_id, config::ss::ServiceNameId_descriptor(), config::ss::SERVICE_NAME_ID_CONFIG);
    if (!config_service_opt) {
        logger().LOG_ERROR("Config service not found.");
        return false;
    }
    config_service_handle_ = *config_service_opt;

    return true;
}

Task<MessagePointer> JSConfigService::OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) {
    // 预加载所有配置
    co_await PreloadAllConfigs();

    logger().LOG_INFO("JSConfigService started and configs preloaded.");
    co_return nullptr;
}

Task<bool> JSConfigService::PreloadAllConfigs() {
    // 获取所有已注册的配置类型
    const auto& settings = imillion().YamlSettings();
    const auto& config_settings = settings["config"];
    if (!config_settings) {
        logger().LOG_ERROR("cannot find 'config'.");
        co_return false;
    }

    const auto& namespace_settings = config_settings["namespace"];
    if (!namespace_settings) {
        logger().LOG_ERROR("cannot find 'config.namespace'.");
        co_return false;
    }
    auto namespace_ = namespace_settings.as<std::string>();

    const auto& tables_message_type_settings = config_settings["tables_message_type"];
    if (!tables_message_type_settings) {
        logger().LOG_ERROR("cannot find 'config.tables_message_type'.");
        co_return false;
    }
    auto tables_message_type = namespace_ + "." + tables_message_type_settings.as<std::string>();
    auto tables_desc = imillion().proto_mgr().FindMessageTypeByName(tables_message_type);
    if (!tables_desc) {
        logger().LOG_ERROR("Unable to find message desc: tables_message_type -> {}.", tables_message_type);
        co_return false;
    }

    for (int i = 0; i < tables_desc->field_count(); ++i) {
        auto field_desc = tables_desc->field(i);
        if (!field_desc || field_desc->type() != google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
            continue;
        }

        const auto* table_desc = field_desc->message_type();
        if (!table_desc) {
            continue;
        }

        // 预加载这个配置类型
        try {
            auto config_resp = co_await Call<config::ConfigQueryReq, config::ConfigQueryResp>(config_service_handle_, *table_desc);
            if (!config_resp->config) {
                logger().LOG_ERROR("Failed to query config: {}", table_desc->full_name());
                continue;
            }

            auto config_table = co_await config_resp->config->Lock(this, config_service_handle_, table_desc);

            if (config_table) {
                // 预转换所有行为JS对象
                std::vector<mjs::Value> cached_rows;
                for (size_t row_idx = 0; row_idx < config_table->GetRowCount(); ++row_idx) {
                    const auto* row = config_table->GetRowByIndex(row_idx);
                    if (row) {
                        auto js_row = JSUtil::ProtoMessageToJSObject(&js_runtime_service_->js_runtime(), *row);
                        cached_rows.push_back(std::move(js_row));
                    }
                }

                logger().LOG_INFO("Preloaded config: {} with {} rows", table_desc->full_name(), cached_rows.size());

                auto cached_table = ConfigTableObject::New(&js_runtime_service_->js_runtime(), table_desc, std::move(cached_rows));
                cached_config_tables_[table_desc] = mjs::Value(cached_table);
                config_name_to_descriptor_[table_desc->full_name()] = table_desc;
            }
        }
        catch (const std::exception& e) {
            logger().LOG_ERROR("Failed to preload config {}: {}", table_desc->full_name(), e.what());
        }
    }
    

    co_return true;
}

Task<mjs::Value> JSConfigService::GetCachedConfigTable(const google::protobuf::Descriptor* descriptor) {
    // 尝试从缓存读取
    auto it = cached_config_tables_.find(descriptor);
    if (it != cached_config_tables_.end()) {
        co_return it->second;
    }
    co_return mjs::Value(); // 不存在返回undefined
}

Task<void> JSConfigService::UpdateConfigCache(const google::protobuf::Descriptor* descriptor) {
    // 配置更新时重新缓存
    logger().LOG_INFO("Config cache updated for: {}", descriptor->full_name());

    co_return;
}




// ConfigModuleObject implementation
ConfigModuleObject::ConfigModuleObject(mjs::Runtime* rt) :
    CppModuleObject(rt)
{
    AddExportMethod(rt, "load", Load);
}

mjs::Value ConfigModuleObject::Load(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSService(context);

    if (par_count < 1) {
        return mjs::Error::Throw(context, "JSConfig Load requires 1 parameter.");
    }

    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "JSConfig Load parameter 1 must be a string for message type.");
    }

    auto msg_name = stack.get(0).string_view();
    const auto* desc = service.imillion().proto_mgr().FindMessageTypeByName(msg_name);
    if (!desc) {
        return mjs::Error::Throw(context, "JSConfig Load parameter 1 invalid message type.");
    }

    // 设置异步操作上下文
    service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(context, mjs::Value()));

    // 从JSRuntimeService获取JSConfigService句柄
    auto& js_runtime_service = service.js_runtime_service();
    service.function_call_context().sender = js_runtime_service.js_config_service_handle();

    // 发送配置查询请求
    service.function_call_context().waiting_session_id = service.Send<JSConfigQueryReq>(
        service.function_call_context().sender, *desc);

    return service.function_call_context().promise;
}


// JSConfigTableClassDef implementation
ConfigTableClassDef::ConfigTableClassDef(mjs::Runtime* runtime)
    : ClassDef(runtime, static_cast<mjs::ClassId>(CustomClassId::kConfigTableObject), "ConfigTable") {

    auto getRowByIndex_const_index = runtime->const_pool().insert(mjs::Value("getRowByIndex"));
    auto findRow_const_index = runtime->const_pool().insert(mjs::Value("findRow"));
    auto getRowCount_const_index = runtime->const_pool().insert(mjs::Value("getRowCount"));

    prototype_.object().SetProperty(nullptr, getRowByIndex_const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
        if (par_count < 1 || !stack.get(0).IsNumber()) {
            return mjs::Error::Throw(context, "getRowByIndex requires a number parameter");
        }

        auto& config_table_object = stack.this_val().object<ConfigTableObject>();
        auto index = static_cast<size_t>(stack.get(0).ToInt64().i64());

        if (index >= config_table_object.cached_rows().size()) {
            return mjs::Value(); // Return undefined for out of bounds
        }

        return config_table_object.cached_rows()[index];
    }));

    prototype_.object().SetProperty(nullptr, findRow_const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
        if (par_count < 1 || !stack.get(0).IsFunctionObject() && !stack.get(0).IsFunctionDef()) {
            return mjs::Error::Throw(context, "findRow requires a function parameter");
        }

        auto& config_table_object = stack.this_val().object<ConfigTableObject>();
        auto predicate_func = stack.get(0);

        // 遍历缓存的JS行对象
        for (const auto& cached_row : config_table_object.cached_rows()) {
            // 调用谓词函数
            std::initializer_list<mjs::Value> args = { cached_row };
            auto result = context->CallFunction(&predicate_func, mjs::Value(), args.begin(), args.end());

            // 检查谓词是否返回true
            if (result.IsBoolean() && result.boolean()) {
                return cached_row;
            }
        }

        return mjs::Value(); // Return undefined if not found
    }));

    prototype_.object().SetProperty(nullptr, getRowCount_const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
        auto& config_table_object = stack.this_val().object<ConfigTableObject>();
        return mjs::Value(config_table_object.cached_rows().size());
    }));
}


// JSConfigTableObject implementation
ConfigTableObject::ConfigTableObject(mjs::Runtime* runtime, const google::protobuf::Descriptor* descriptor, std::vector<mjs::Value>&& cached_rows)
    : Object(runtime, static_cast<mjs::ClassId>(CustomClassId::kConfigTableObject))
    , descriptor_(descriptor)
    , cached_rows_(std::move(cached_rows)) {
}



// ConfigTableWeakClassDef implementation
ConfigTableWeakClassDef::ConfigTableWeakClassDef(mjs::Runtime* runtime) 
    : ClassDef(runtime, static_cast<mjs::ClassId>(CustomClassId::kConfigTableWeakObject), "ConfigTableWeak") {

    auto lock_const_index = runtime->const_pool().insert(mjs::Value("lock"));

    prototype_.object().SetProperty(nullptr, lock_const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
        auto& service = GetJSService(context);
        auto& config_table_weak_object = stack.this_val().object<ConfigTableWeakObject>();

        // Get config service handle
        auto config_service_handle = service.js_runtime_service().config_service_handle();

        auto& weak_obj = stack.this_val().object<ConfigTableWeakObject>();
        
        // 这里未来需要修改C++消息类型，区分初次加载和加锁重载
        auto obj = weak_obj.Lock(context);
        return obj;
    }));
}

// ConfigTableWeakObject implementation
ConfigTableWeakObject::ConfigTableWeakObject(mjs::Context* context, mjs::Value&& table_object)
    : Object(context, static_cast<mjs::ClassId>(CustomClassId::kConfigTableWeakObject))
    , table_object_(std::move(table_object)) {}

mjs::Value ConfigTableWeakObject::Lock(mjs::Context* context) {
    auto& obj = table_object_.object<ConfigTableObject>();
    if (!obj.is_expired()) {
        return table_object_;
    }
    auto& service = GetJSService(context);

    // 重新加载
    service.function_call_context().sender = service.js_runtime_service().js_config_service_handle();

    service.function_call_context().waiting_session_id = service.Send<JSConfigQueryReq>(
        service.function_call_context().sender, *obj.descriptor());

    service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(context, mjs::Value()));
    return service.function_call_context().promise;
}



} // namespace jssvr
} // namespace million