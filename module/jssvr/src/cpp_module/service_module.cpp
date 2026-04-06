#include "service_module.h"

#include <million/jssvr/jssvr.h>

#include "../js_service.h"
#include "../js_util.h"

namespace million {
namespace jssvr {

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

} // namespace jssvr
} // namespace million
