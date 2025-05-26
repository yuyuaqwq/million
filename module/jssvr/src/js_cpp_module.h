#pragma once

#include <mjs/runtime.h>
#include <mjs/context.h>
#include <mjs/object_impl/cpp_module_object.h>

#include <million/imillion.h>

namespace million {
namespace jssvr {


class MillionModuleObject : public mjs::CppModuleObject {
public:
    MillionModuleObject(mjs::Runtime* rt);

    static mjs::Value NewService(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
};

class ServiceModuleObject : public mjs::CppModuleObject {
public:
    ServiceModuleObject(mjs::Runtime* rt);
};

class LoggerModuleObject : public mjs::CppModuleObject {
public:
    LoggerModuleObject(mjs::Runtime* rt);

    static mjs::Value Debug(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value Info(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);
    static mjs::Value Error(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack);

private:
    static million::SourceLocation GetSourceLocation(const mjs::StackFrame& stack);
};

} // namespace jssvr
} // namespace million