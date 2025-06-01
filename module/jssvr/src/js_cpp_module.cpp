#include "js_cpp_module.h"

#include <jssvr/jssvr.h>
#include "js_service.h"
#include "js_util.h"

namespace million {
namespace jssvr {

MillionModuleObject::MillionModuleObject(mjs::Runtime* rt)
    : CppModuleObject(rt)
{
    AddExportMethod(rt, "newservice", NewService);
    AddExportMethod(rt, "makemsg", MakeMsg);
}

mjs::Value MillionModuleObject::NewService(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSRuntineService(&context->runtime());
    if (par_count < 1) {
        return mjs::Error::Throw(context, "Creating a service requires 1 parameter.");
    }
    NewJSService(&service.imillion(), stack.get(0).ToString(context).string_view());
    return mjs::Value();
}

mjs::Value MillionModuleObject::MakeMsg(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    if (par_count < 2) {
        return mjs::Error::Throw(context, "MakeMsg requires 2 parameter.");
    }
    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "Message parameter 1 must be a string for creation.");
    }
    if (!stack.get(1).IsObject()) {
        return mjs::Error::Throw(context, "Message parameter 2 must be a object for creation.");
    }

    auto array = mjs::ArrayObject::New(context, 2);
    array->operator[](0) = std::move(stack.get(0));
    array->operator[](1) = std::move(stack.get(1));

    return mjs::Value(array);
}

ServiceModuleObject::ServiceModuleObject(mjs::Runtime* rt)
    : CppModuleObject(rt)
{
    AddExportMethod(rt, "send", Send);
    AddExportMethod(rt, "call", Call);
}

mjs::Value ServiceModuleObject::Send(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto result = Call(context, par_count, stack);
    auto& service = GetJSService(context);
    service.function_call_context().waiting_session_id.reset();
    return result;
}

mjs::Value ServiceModuleObject::Call(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSService(context);
    if (par_count < 3) {
        return mjs::Error::Throw(context, "Send requires 2 parameter.");
    }

    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "The service name is not a string.");
    }
    if (!stack.get(1).IsString()) {
        return mjs::Error::Throw(context, "The message name is not a string.");
    }
    if (!stack.get(2).IsObject()) {
        return mjs::Error::Throw(context, "The message is not an object.");
    }

    auto target = service.imillion().GetServiceByName(stack.get(0).string_view());
    if (!target) {
        return mjs::Value();
    }

    auto desc = service.imillion().proto_mgr().FindMessageTypeByName(stack.get(1).string_view());
    if (!desc) {
        return mjs::Value();
    }
    auto msg = service.imillion().proto_mgr().NewMessage(*desc);

    JSUtil::JSObjectToProtoMessage(context, msg.get(), stack.get(2));

    service.function_call_context().waiting_session_id = service.Send(*target, MessagePointer(std::move(msg)));
    service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(context, mjs::Value()));
    return service.function_call_context().promise;
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

// ConfigModuleObject implementation
ConfigModuleObject::ConfigModuleObject(mjs::Runtime* rt) :
    CppModuleObject(rt)
{
    AddExportMethod(rt, "load", Load);
}

mjs::Value ConfigModuleObject::Load(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSService(context);

    if (par_count < 1) {
        return mjs::Error::Throw(context, "Config Load requires 1 parameter.");
    }

    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "Config Load parameter 1 must be a string for message type.");
    }

    auto msg_name = stack.get(0).string_view();

    const auto* desc = service.imillion().proto_mgr().FindMessageTypeByName(msg_name);
    if (!desc) {
        return mjs::Error::Throw(context, "Config Load parameter 1 invalid message type.");
    }

    // Get config service handle
    auto config_service_opt = service.imillion().GetServiceByName(config::kConfigServiceName);
    if (!config_service_opt) {
        return mjs::Error::Throw(context, "Config service not found.");
    }
    auto config_service = *config_service_opt;
    
    // Start async config load operation
    service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(context, mjs::Value()));
    

    // 这里让OnMsg等待，发现是C++消息再做分发

    service.function_call_context().sender = service.js_runtime_service().config_service_handle();

    service.function_call_context().waiting_session_id = service.Send<config::ConfigQueryReq>(service.function_call_context().sender, *desc);
    return service.function_call_context().promise;
}



// ConfigTableClassDef implementation
ConfigTableClassDef::ConfigTableClassDef(mjs::Runtime* runtime) 
    : ClassDef(runtime, static_cast<mjs::ClassId>(CustomClassId::kConfigTableObject), "ConfigTable") {

    auto getRowByIndex_const_index = runtime->const_pool().insert(mjs::Value("getRowByIndex"));
    auto findRow_const_index = runtime->const_pool().insert(mjs::Value("findRow"));

    prototype_.object().SetProperty(nullptr, getRowByIndex_const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
        if (par_count < 1 || !stack.get(0).IsNumber()) {
            return mjs::Error::Throw(context, "getRowByIndex requires a number parameter");
        }
        
        auto& config_table_object = stack.this_val().object<ConfigTableObject>();
        auto index = static_cast<size_t>(stack.get(0).ToInt64().i64());
        
        if (index >= config_table_object.config_table()->GetRowCount()) {
            return mjs::Value(); // Return undefined for out of bounds
        }
        
        const auto* row = config_table_object.config_table()->GetRowByIndex(index);
        if (!row) {
            return mjs::Value(); // Return undefined if not found
        }
        
        return JSUtil::ProtoMessageToJSObject(context, *row);
    }));

    prototype_.object().SetProperty(nullptr, findRow_const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
        if (par_count < 1 || !stack.get(0).IsFunctionObject() && !stack.get(0).IsFunctionDef()) {
            return mjs::Error::Throw(context, "findRow requires a function parameter");
        }
        
        auto& config_table_object = stack.this_val().object<ConfigTableObject>();
        auto predicate_func = stack.get(0);
        
        // Iterate through all rows
        for (size_t i = 0; i < config_table_object.config_table()->GetRowCount(); ++i) {
            const auto* row = config_table_object.config_table()->GetRowByIndex(i);
            if (!row) continue;
            
            // Convert row to JS object
            auto row_js = JSUtil::ProtoMessageToJSObject(context, *row);
            
            // Call predicate function with row
            std::vector<mjs::Value> args = { std::move(row_js) };
            auto result = context->CallFunction(&predicate_func, mjs::Value(), args.begin(), args.end());
            
            // Check if predicate returned true
            if (result.IsBoolean() && result.boolean()) {
                return JSUtil::ProtoMessageToJSObject(context, *row);
            }
        }
        
        return mjs::Value(); // Return undefined if not found
    }));
}



// ConfigTableObject implementation
ConfigTableObject::ConfigTableObject(mjs::Context* context, config::ConfigTableShared config_table)
    : Object(context, static_cast<mjs::ClassId>(CustomClassId::kConfigTableObject))
    , config_table_(std::move(config_table)) {}


// ConfigTableWeakClassDef implementation
ConfigTableWeakClassDef::ConfigTableWeakClassDef(mjs::Runtime* runtime) 
    : ClassDef(runtime, static_cast<mjs::ClassId>(CustomClassId::kConfigTableWeakObject), "ConfigTableWeak") {

    auto lock_const_index = runtime->const_pool().insert(mjs::Value("lock"));

    prototype_.object().SetProperty(nullptr, lock_const_index, mjs::Value([](mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
        auto& service = GetJSService(context);
        auto& config_table_weak_object = stack.this_val().object<ConfigTableWeakObject>();

        // Get config service handle
        auto config_service_handle = service.js_runtime_service().config_service_handle();

        // Start async lock operation
        service.function_call_context().sender = config_service_handle;
        service.function_call_context().waiting_session_id = service.Send<config::ConfigQueryReq>(config_service_handle, 
            *config_table_weak_object.descriptor());

        service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(context, mjs::Value()));
        return service.function_call_context().promise;
    }));
}

// ConfigTableWeakObject implementation
ConfigTableWeakObject::ConfigTableWeakObject(mjs::Context* context, config::ConfigTableWeakBase config_table_weak, const google::protobuf::Descriptor* descriptor)
    : Object(context, static_cast<mjs::ClassId>(CustomClassId::kConfigTableWeakObject))
    , config_table_weak_(std::move(config_table_weak))
    , descriptor_(descriptor) {}

} // namespace jssvr
} // namespace million