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
    // ���캯�������ܸ�ʽ���Ŀɱ����
    // �ǲ�����Ҫ�ĵ���Ϊɶfmt::print����
    template <typename... Args>
    explicit TaskAbortException(const std::string& message, Args&&... args)
        : message_(std::vformat(message, std::make_format_args(std::forward<Args>(args)...))) {

        // �������ջ��Ϣ
#ifdef MILLION_STACK_TRACE
        stacktrace_ = std::stacktrace::current();
#endif
    }

    // ��ȡ������Ϣ
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