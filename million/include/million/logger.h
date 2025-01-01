#pragma once

#include <string>
#include <format>
#include <source_location>

#include <million/msg.h>
#include <million/service_handle.h>

#include <ss/ss_logger.pb.h>

namespace million {

class Million;
class MILLION_API Logger {
public:
    Logger(Million* million);
    ~Logger();

    bool Init();

    template <typename... Args>
    void Log(const std::source_location& source, ss::logger::LogLevel level, const std::string& str) {
        Log(logger_handle_, source, level, str);
    }

    void SetLevel(ss::logger::LogLevel level) {
        SetLevel(logger_handle_, level);
    }

private:
    void Log(const ServiceHandle& sender, const std::source_location& source, ss::logger::LogLevel level, const std::string& str);
    void SetLevel(const ServiceHandle& sender, ss::logger::LogLevel level);

private:
    Million* million_;
    ServiceHandle logger_handle_;
};

#define Trace(fmt, ...) Log(std::source_location::current(), ::million::ss::logger::LOG_LEVEL_TRACE, ::std::format(fmt, __VA_ARGS__))
#define Debug(fmt, ...) Log(std::source_location::current(), ::million::ss::logger::LOG_LEVEL_DEBUG, ::std::format(fmt, __VA_ARGS__))
#define Info(fmt, ...) Log(std::source_location::current(), ::million::ss::logger::LOG_LEVEL_INFO, ::std::format(fmt, __VA_ARGS__))
#define Warn(fmt, ...) Log(std::source_location::current(), ::million::ss::logger::LOG_LEVEL_WARN, ::std::format(fmt, __VA_ARGS__))
#define Err(fmt, ...) Log(std::source_location::current(), ::million::ss::logger::LOG_LEVEL_ERR, ::std::format(fmt, __VA_ARGS__))
#define Critical(fmt, ...) Log(std::source_location::current(), ::million::ss::logger::LOG_LEVEL_CRITICAL, ::std::format(fmt, __VA_ARGS__))
#define Off(fmt, ...) Log(std::source_location::current(), ::million::ss::logger::LOG_LEVEL_OFF, ::std::format(fmt, __VA_ARGS__))

} // namespace million
