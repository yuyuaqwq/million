#include "js_cpp_module.h"

#include <jssvr/jssvr.h>
#include "js_service.h"
#include "js_util.h"

#include <config/config.h>

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

void DBRowObject::SetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value&& value) {
    auto key_str = context->runtime().const_table().at(key).string_view();
    
    // Special method handling
    if (key_str == "commit") {
        return; // commit is a method, not a property to set
    }
    
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
    auto key_str = context->runtime().const_table().at(key).string_view();
    
    // Special method handling
    if (key_str == "commit") {
        // Return a function that calls DBRow::Commit
        auto commit_func = mjs::CppFunction::New(context, [this](mjs::Context* ctx, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
            auto& service = GetJSService(ctx);
            
            // Start async commit operation
            service.function_call_context().promise = mjs::Value(mjs::PromiseObject::New(ctx, mjs::Value()));
            
            // Schedule commit task
            auto commit_task = [&service, this]() -> Task<> {
                auto db_service = service.js_runtime_service().db_service_handle();
                co_await db_row_.Commit(&service, db_service);
                
                // Resolve promise
                if (service.function_call_context().promise.IsPromiseObject()) {
                    service.function_call_context().promise.promise().Resolve(ctx, mjs::Value());
                }
            };
            
            service.imillion().StartTask(commit_task());
            
            return service.function_call_context().promise;
        });
        
        *value = mjs::Value(commit_func);
        return true;
    }
    
    const auto& desc = db_row_.GetDescriptor();
    const auto& reflection = db_row_.GetReflection();
    
    // Find the field by name
    const auto* field = desc.FindFieldByName(key_str);
    if (!field) {
        return false; // Field not found
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
    
    // Schedule config query task
    auto config_task = [&service, config_service, desc, context]() -> Task<> {
        try {
            auto resp = co_await service.Call<config::ConfigQueryReq, config::ConfigQueryResp>(config_service, *desc);
            if (!resp->config) {
                // Reject promise
                if (service.function_call_context().promise.IsPromiseObject()) {
                    service.function_call_context().promise.promise().Reject(context, mjs::Value("Config not found"));
                }
                co_return;
            }
            
            // Lock the config table
            auto config_table = co_await resp->config->Lock(&service, config_service, desc);
            
            // Create ConfigTableObject wrapper
            auto config_table_obj = ConfigTableObject::New(context, std::move(config_table));
            
            // Resolve promise with config table object
            if (service.function_call_context().promise.IsPromiseObject()) {
                service.function_call_context().promise.promise().Resolve(context, mjs::Value(config_table_obj));
            }
        } catch (const std::exception& e) {
            // Reject promise on error
            if (service.function_call_context().promise.IsPromiseObject()) {
                service.function_call_context().promise.promise().Reject(context, mjs::Value(e.what()));
            }
        }
    };
    
    service.imillion().StartTask(config_task());
    
    return service.function_call_context().promise;
}

// ConfigTableObject implementation
ConfigTableObject::ConfigTableObject(mjs::Context* context, config::ConfigTableShared config_table)
    : Object(context), config_table_(std::move(config_table))
{
}

void ConfigTableObject::SetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value&& value) {
    // Config tables are read-only, ignore set operations
}

bool ConfigTableObject::GetProperty(mjs::Context* context, mjs::ConstIndex key, mjs::Value* value) {
    auto key_str = context->runtime().const_table().at(key).string_view();
    
    if (key_str == "getRowByIndex") {
        auto getRowByIndex_func = mjs::CppFunction::New(context, [this](mjs::Context* ctx, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
            if (par_count < 1 || !stack.get(0).IsNumber()) {
                return mjs::Error::Throw(ctx, "getRowByIndex requires a number parameter");
            }
            
            auto index = static_cast<size_t>(stack.get(0).ToInt64().i64());
            if (index >= config_table_->GetRowCount()) {
                return mjs::Value(); // Return undefined for out of bounds
            }
            
            const auto* row = config_table_->GetRowByIndex(index);
            if (!row) {
                return mjs::Value(); // Return undefined if not found
            }
            
            return JSUtil::ProtoMessageToJSObject(ctx, *row);
        });
        
        *value = mjs::Value(getRowByIndex_func);
        return true;
    }
    
    if (key_str == "findRow") {
        auto findRow_func = mjs::CppFunction::New(context, [this](mjs::Context* ctx, uint32_t par_count, const mjs::StackFrame& stack) -> mjs::Value {
            if (par_count < 1 || !stack.get(0).IsFunction()) {
                return mjs::Error::Throw(ctx, "findRow requires a function parameter");
            }
            
            auto predicate_func = stack.get(0);
            
            // Iterate through all rows
            for (size_t i = 0; i < config_table_->GetRowCount(); ++i) {
                const auto* row = config_table_->GetRowByIndex(i);
                if (!row) continue;
                
                // Convert row to JS object
                auto row_js = JSUtil::ProtoMessageToJSObject(ctx, *row);
                
                // Call predicate function with row
                std::vector<mjs::Value> args = { std::move(row_js) };
                auto result = ctx->CallFunction(&predicate_func, mjs::Value(), args.begin(), args.end());
                
                // Check if predicate returned true
                if (result.IsBoolean() && result.boolean()) {
                    return JSUtil::ProtoMessageToJSObject(ctx, *row);
                }
            }
            
            return mjs::Value(); // Return undefined if not found
        });
        
        *value = mjs::Value(findRow_func);
        return true;
    }
    
    return false; // Property not found
}

} // namespace jssvr
} // namespace million