#pragma once

#include <string>
#include <format>
#include <source_location>

#include <million/msg.h>
#include <million/service_handle.h>

namespace million {

class Million;
class MILLION_API Logger {
public:
    enum class LogLevel{
        kTrace,
        kDebug,
        kInfo,
        kWarn,
        kErr,
        kCritical,
        kOff,
    };

public:
    Logger(Million* million);
    ~Logger();

    bool Init();

    void BindService(ServiceHandle logger_svr_handle) {
        logger_svr_handle_ = logger_svr_handle;
        is_bind_ = true;
    }

    template <typename... Args>
    void Log(const std::source_location& source, LogLevel level, const std::string& str) {
        Log(logger_svr_handle_, source, level, str);
    }

    void SetLevel(LogLevel level) {
        SetLevel(logger_svr_handle_, level);
    }

private:
    void Log(const ServiceHandle& sender, const std::source_location& source, LogLevel level, const std::string& str);
    void SetLevel(const ServiceHandle& sender, LogLevel level);

private:
    Million* million_;
    bool is_bind_ = false;
    ServiceHandle logger_svr_handle_;
};

MILLION_MSG_DEFINE(MILLION_API, LoggerLog, (const std::source_location) source, (Logger::LogLevel) level, (const std::string) msg);
MILLION_MSG_DEFINE(MILLION_API, LoggerSetLevel, (Logger::LogLevel) new_level);

#define Trace(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kTrace, ::std::format(fmt, __VA_ARGS__))
#define Debug(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kDebug, ::std::format(fmt, __VA_ARGS__))
#define Info(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kInfo, ::std::format(fmt, __VA_ARGS__))
#define Warn(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kWarn, ::std::format(fmt, __VA_ARGS__))
#define Err(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kErr, ::std::format(fmt, __VA_ARGS__))
#define Critical(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kCritical, ::std::format(fmt, __VA_ARGS__))
#define Off(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kOff, ::std::format(fmt, __VA_ARGS__))

} // namespace million
