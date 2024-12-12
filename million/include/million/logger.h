#pragma once

#include <string>
#include <format>
#include <source_location>

#include <million/imsg.h>
#include <million/service_handle.h>

namespace million {

enum class LogLevel : int {
    kTrace = 0,     // trace
    kDebug = 1,     // debug
    kInfo = 2,      // info
    kWarn = 3,      // warn
    kErr = 4,       // err
    kCritical = 5,  // critical
    kOff = 6,       // off
};

class Million;
class MILLION_API Logger {
public:
    Logger(Million* million);
    ~Logger();

    bool Init();

    template <typename... Args>
    void Log(const std::source_location& source, LogLevel level, const std::string& str) {
        Log(logger_handle_, source, level, str);
    }

    void SetLevel(std::string_view level) {
        SetLevel(logger_handle_, std::string(level));
    }

private:
    void Log(const ServiceHandle& sender, const std::source_location& source, LogLevel level, const std::string& str);
    void SetLevel(const ServiceHandle& sender, const std::string& level);

private:
    Million* million_;
    ServiceHandle logger_handle_;
};

#define Trace(fmt, ...) Log(std::source_location::current(), ::million::LogLevel::kTrace, ::std::format(fmt, __VA_ARGS__))
#define Debug(fmt, ...) Log(std::source_location::current(), ::million::LogLevel::kDebug, ::std::format(fmt, __VA_ARGS__))
#define Info(fmt, ...) Log(std::source_location::current(), ::million::LogLevel::kInfo, ::std::format(fmt, __VA_ARGS__))
#define Warn(fmt, ...) Log(std::source_location::current(), ::million::LogLevel::kWarn, ::std::format(fmt, __VA_ARGS__))
#define Err(fmt, ...) Log(std::source_location::current(), ::million::LogLevel::kErr, ::std::format(fmt, __VA_ARGS__))
#define Critical(fmt, ...) Log(std::source_location::current(), ::million::LogLevel::kCritical, ::std::format(fmt, __VA_ARGS__))
#define Off(fmt, ...) Log(std::source_location::current(), ::million::LogLevel::kOff, ::std::format(fmt, __VA_ARGS__))

} // namespace million
