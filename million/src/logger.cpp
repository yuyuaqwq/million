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
        InitLog(level, short_func.c_str(), source.line(), msg.c_str());
    }
    else {
        million_->imillion().Send<LoggerLog>(sender, logger_svr_handle_, source, level, msg);
    }
}

void Logger::Log(const ServiceHandle& sender, SourceLocation source, LogLevel level, const std::string& msg) {
    if (!is_bind_) {
        std::string short_func = ExtractFunctionName(source.function_name);
        InitLog(level, short_func.c_str(), source.line, msg.c_str());
    }
    else {
        million_->imillion().Send<LoggerLog2>(sender, logger_svr_handle_, source, level, msg);
    }
}

void Logger::InitLog(LogLevel level, const char* function_name, uint32_t line,const char* msg) {
    switch (level) {
    case LogLevel::kTrace:
        std::cout << "[million] [init] [trace] " << "[" << function_name << ":" << line << "] " << msg << std::endl;
        break;
    case LogLevel::kDebug:
        std::cout << "[million] [init] [debug] " << "[" << function_name << ":" << line << "] " << msg << std::endl;
        break;
    case LogLevel::kInfo:
        std::cout << "[million] [init] [info] " << "[" << function_name << ":" << line << "] " << msg << std::endl;
        break;
    case LogLevel::kWarn:
        std::cerr << "[million] [init] [warn] " << "[" << function_name << ":" << line << "] " << msg << std::endl;
        break;
    case LogLevel::kError:
        std::cerr << "[million] [init] [error] " << "[" << function_name << ":" << line << "] " << msg << std::endl;
        break;
    case LogLevel::kCritical:
        std::cerr << "[million] [init] [critical] " << "[" << function_name << ":" << line << "] " << msg << std::endl;
        break;
    }
}


void Logger::SetLevel(const ServiceHandle& sender, LogLevel level) {
    million_->imillion().Send<LoggerSetLevel>(sender, logger_svr_handle_, level);
}

} // namespace million