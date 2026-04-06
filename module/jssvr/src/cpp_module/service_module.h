#pragma once

#include <mjs/runtime.h>
#include <mjs/context.h>
#include <mjs/object/cpp_module_object.h>

namespace million {
namespace jssvr {

class ServiceModuleObject : public mjs::CppModuleObject {
private:
    ServiceModuleObject(mjs::Runtime* rt);

public:
    static mjs::Value Send(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value Call(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value Reply(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

    static ServiceModuleObject* New(mjs::Runtime* runtime) {
        return new ServiceModuleObject(runtime);
    }
};

} // namespace jssvr
} // namespace million
