#pragma once

#include <string>
#include <format>
#include <exception>

// #define MILLION_STACK_TRACE
#ifdef MILLION_STACK_TRACE
#include <stacktrace>
#endif

namespace million {

class TaskAbortException : public std::exception {
public:
    // 构造函数，接受格式化的可变参数
    // 是不是需要改掉？为啥fmt::print可以
    template <typename... Args>
    explicit TaskAbortException(const std::string& message, Args&&... args)
        : message_(std::vformat(message, std::make_format_args(std::forward<Args>(args)...))) {

        // 捕获调用栈信息
#ifdef MILLION_STACK_TRACE
        stacktrace_ = std::stacktrace::current();
#endif
    }

    // 获取错误信息
    const char* what() const noexcept override {
        return message_.c_str();
    }

#ifdef MILLION_STACK_TRACE
    const std::stacktrace& stacktrace() const { return stacktrace_; }
#endif

private:
    std::string message_;
#ifdef MILLION_STACK_TRACE
    std::stacktrace stacktrace_;
#endif
};

} // namespace million