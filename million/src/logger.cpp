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

void Logger::Log(const ServiceHandle& sender, const std::source_location& source, Level level, const std::string& msg) {
    million_->imillion().Send<LoggerLog>(sender, logger_svr_handle_, source, level, msg);
}

void Logger::SetLevel(const ServiceHandle& sender, Level level) {
    million_->imillion().Send<LoggerSetLevel>(sender, logger_svr_handle_, level);
}

} // namespace million