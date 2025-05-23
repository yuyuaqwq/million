#pragma once

#include <string>
#include <format>
#include <source_location>

#include <million/message.h>
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
        kError,
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

    static std::string ExtractFunctionName(std::string_view full_name) {
        auto paren_pos = full_name.find_last_of('(');
        if (paren_pos == std::string_view::npos) {
            return std::string(full_name);
        }

        auto space_pos = full_name.find_last_of(' ', paren_pos);
        if (space_pos == std::string_view::npos) {
            return std::string(full_name.substr(paren_pos));
        }
        ++space_pos;

        return std::string(full_name.substr(space_pos, paren_pos - space_pos));
    }

private:
    void Log(const ServiceHandle& sender, const std::source_location& source, LogLevel level, const std::string& str);
    void SetLevel(const ServiceHandle& sender, LogLevel level);

private:
    Million* million_;
    bool is_bind_ = false;
    ServiceHandle logger_svr_handle_;
};

MILLION_MESSAGE_DEFINE(MILLION_API, LoggerLog, (const std::source_location) source, (Logger::LogLevel) level, (const std::string) msg);
MILLION_MESSAGE_DEFINE(MILLION_API, LoggerSetLevel, (Logger::LogLevel) new_level);

#define LOG_TRACE(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kTrace, ::std::format(fmt, __VA_ARGS__))
#define LOG_DEBUG(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kDebug, ::std::format(fmt, __VA_ARGS__))
#define LOG_INFO(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kInfo, ::std::format(fmt, __VA_ARGS__))
#define LOG_WARN(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kWarn, ::std::format(fmt, __VA_ARGS__))
#define LOG_ERROR(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kError, ::std::format(fmt, __VA_ARGS__))
#define LOG_CRITICAL(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kCritical, ::std::format(fmt, __VA_ARGS__))
#define LOG_OFF(fmt, ...) Log(std::source_location::current(), ::million::Logger::LogLevel::kOff, ::std::format(fmt, __VA_ARGS__))

} // namespace million
