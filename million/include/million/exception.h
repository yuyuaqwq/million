#pragma once

#include <string>
#include <format>
#include <exception>

//#define MILLION_STACK_TRACE
#ifdef MILLION_STACK_TRACE
#include <stacktrace>
#endif

#include <million/api.h>

namespace million {

class MILLION_CLASS_API TaskAbortException : public std::exception {
public:
    // ���캯�������ܸ�ʽ���Ŀɱ����

    explicit TaskAbortException(const std::string& message)
        : message_(message) {

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

#define THROW_TaskAbortException(fmt, ...)  throw TaskAbortException(::std::format(fmt, __VA_ARGS__))

} // namespace million