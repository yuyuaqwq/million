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
        switch (level) {
            case LogLevel::kTrace:
                std::cout << "[million] [init] [trace] " << "[" << source.function_name() << "] " << msg << std::endl;
                break;
            case LogLevel::kDebug:
                std::cout << "[million] [init] [debug] " << "[" << source.function_name() << "] " << msg << std::endl;
                break;
            case LogLevel::kInfo:
                std::cout << "[million] [init] [info] " << "[" << source.function_name() << "] " << msg << std::endl;
                break;
            case LogLevel::kWarn:
                std::cerr << "[million] [init] [warn] " << "[" << source.function_name() << "] " << msg << std::endl;
                break;
            case LogLevel::kErr:
                std::cerr << "[million] [init] [err] " << "[" << source.function_name() << "] " << msg << std::endl;
                break;
            case LogLevel::kCritical:
                std::cerr << "[million] [init] [critical] " << "[" << source.function_name() << "] " << msg << std::endl;
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