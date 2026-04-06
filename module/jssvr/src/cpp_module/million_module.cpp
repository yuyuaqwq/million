#include "million_module.h"

#include <million/jssvr/jssvr.h>
#include <million/jssvr/ss_jssvr.pb.h>

#include "../js_service.h"

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
    array->At(context, 0) = std::move(stack.get(0));
    array->At(context, 1) = std::move(stack.get(1));

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

} // namespace jssvr
} // namespace million
