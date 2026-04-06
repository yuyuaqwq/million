#pragma once

#include <mjs/runtime.h>
#include <mjs/context.h>
#include <mjs/object/cpp_module_object.h>

namespace million {
namespace jssvr {

class TLogModuleObject : public mjs::CppModuleObject {
private:
    TLogModuleObject(mjs::Runtime* rt);

public:
    static mjs::Value Info(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value Critical(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value Stat(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value GetStats(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

    static TLogModuleObject* New(mjs::Runtime* runtime) {
        return new TLogModuleObject(runtime);
    }
};

} // namespace jssvr
} // namespace million
