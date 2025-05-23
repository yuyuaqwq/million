#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <million/logger.h>

#include "million.h"

namespace million {
Logger::Logger(Million* million)
    : million_(million) {}

Logger::~Logger() = default;

bool Logger::Init() {
    return true;
}

void Logger::Log(const ServiceHandle& sender, const std::source_location& source, LogLevel level, const std::string& msg) {
    if (!is_bind_) {
        std::string short_func = ExtractFunctionName(source.function_name());
        switch (level) {
            case LogLevel::kTrace:
                std::cout << "[million] [init] [trace] " << "[" << short_func << ":" << source.line() << "] " << msg << std::endl;
                break;
            case LogLevel::kDebug:
                std::cout << "[million] [init] [debug] " << "[" << short_func << ":" << source.line() << "] " << msg << std::endl;
                break;
            case LogLevel::kInfo:
                std::cout << "[million] [init] [info] " << "[" << short_func << ":" << source.line() << "] " << msg << std::endl;
                break;
            case LogLevel::kWarn:
                std::cerr << "[million] [init] [warn] " << "[" << short_func << ":" << source.line() << "] " << msg << std::endl;
                break;
            case LogLevel::kError:
                std::cerr << "[million] [init] [error] " << "[" << short_func << ":" << source.line() << "] " << msg << std::endl;
                break;
            case LogLevel::kCritical:
                std::cerr << "[million] [init] [critical] " << "[" << short_func << ":" << source.line() << "] " << msg << std::endl;
                break;
        }
    }
    else {
        million_->imillion().Send<LoggerLog>(sender, logger_svr_handle_, source, level, msg);
    }
}

void Logger::SetLevel(const ServiceHandle& sender, LogLevel level) {
    million_->imillion().Send<LoggerSetLevel>(sender, logger_svr_handle_, level);
}

} // namespace million