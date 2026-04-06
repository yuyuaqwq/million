#pragma once

#include <mjs/runtime.h>
#include <mjs/context.h>
#include <mjs/object/cpp_module_object.h>
#include <million/imillion.h>

namespace million {
namespace jssvr {

// JS服务超时消息
MILLION_MESSAGE_DEFINE(, JsServiceTimeoutMessage, (uint64_t) tick, (mjs::Value) function)

class MillionModuleObject : public mjs::CppModuleObject {
private:
    MillionModuleObject(mjs::Runtime* rt);

public:
    static mjs::Value NewService(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value MakeMessage(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value Timeout(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

    static MillionModuleObject* New(mjs::Runtime* runtime) {
        return new MillionModuleObject(runtime);
    }
};

} // namespace jssvr
} // namespace million
