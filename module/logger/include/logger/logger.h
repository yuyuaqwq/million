#pragma once

#include <format>

#include <million/imsg.h>

namespace million {
namespace logger {

enum LogLevel : int {
    kTrace = 0,     // trace
    kDebug = 1,     // debug
    kInfo = 2,      // info
    kWarn = 3,      // warn
    kErr = 4,       // err
    kCritical = 5,  // critical
    kOff = 6,       // off
};

MILLION_MSG_DEFINE(LoggerLogMsg, (LogLevel) level, (const char*) file, (int) line, (const char*) function, (std::string) info);
MILLION_MSG_DEFINE(LoggerSetLevelMsg, (std::string) level);

extern MILLION_OBJECT_IMPORT ServiceHandle logger_handle;

#define MILLION_LOGGER_CALL(MILLION_, SENDER_, LEVEL_, FMT_, ...) \
        MILLION_->Send<::million::logger::LoggerLogMsg>(SENDER_, ::million::logger::logger_handle, LEVEL_, __FILE__, __LINE__, __func__,  ::std::format(FMT_, __VA_ARGS__))

#define MILLION_SERVICE_LOGGER_CALL(LEVEL_, FMT_, ...) \
        Send<::million::logger::LoggerLogMsg>(::million::logger::logger_handle, LEVEL_, __FILE__, __LINE__, __func__,  ::std::format(FMT_, __VA_ARGS__))

#define LOG_TRACE(...) MILLION_SERVICE_LOGGER_CALL(::million::logger::LogLevel::kTrace, __VA_ARGS__)
#define LOG_DEBUG(...) MILLION_SERVICE_LOGGER_CALL(::million::logger::LogLevel::kDebug, __VA_ARGS__)
#define LOG_INFO(...) MILLION_SERVICE_LOGGER_CALL(::million::logger::LogLevel::kInfo, __VA_ARGS__)
#define LOG_WARN(...) MILLION_SERVICE_LOGGER_CALL(::million::logger::LogLevel::kWarn, __VA_ARGS__)
#define LOG_ERR(...) MILLION_SERVICE_LOGGER_CALL(::million::logger::LogLevel::kErr, __VA_ARGS__)
#define LOG_CRITICAL(...) MILLION_SERVICE_LOGGER_CALL(::million::logger::LogLevel::kCritical, __VA_ARGS__)

} // namespace gateway
} // namespace million
