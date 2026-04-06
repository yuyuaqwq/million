#include "logger_module.h"

#include <million/jssvr/jssvr.h>

#include "../js_service.h"

namespace million {
namespace jssvr {

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

} // namespace jssvr
} // namespace million
