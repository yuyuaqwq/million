#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#include <spdlog/sinks/hourly_file_sink.h>

#include <million/logger/logger.h>

#include "million.h"
#include "logger.h"

namespace million {

MILLION_MSG_DEFINE(, LoggerLogMsg, (logger::LogLevel)level, (const char*)file, (int)line, (const char*)function, (std::string)info);
MILLION_MSG_DEFINE(, LoggerSetLevelMsg, (std::string)level);

static_assert(logger::LogLevel::kTrace == spdlog::level::level_enum::trace);
static_assert(logger::LogLevel::kDebug == spdlog::level::level_enum::debug);
static_assert(logger::LogLevel::kInfo == spdlog::level::level_enum::info);
static_assert(logger::LogLevel::kWarn == spdlog::level::level_enum::warn);
static_assert(logger::LogLevel::kErr == spdlog::level::level_enum::err);
static_assert(logger::LogLevel::kCritical == spdlog::level::level_enum::critical);
static_assert(logger::LogLevel::kOff == spdlog::level::level_enum::off);

class LoggerService : public IService {
public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override {
        auto& config = imillion_->YamlConfig();
        auto logger_config = config["logger"];
        if (!logger_config) {
            std::cerr << "[config][error]cannot find 'logger'." << std::endl;
            return false;
        }
        if (!logger_config["log_file"]) {
            std::cerr << "[config][error]cannot find 'logger.log_file'." << std::endl;
            return false;
        }
        auto log_file = logger_config["log_file"].as<std::string>();

        if (!logger_config["level"]) {
            std::cerr << "[config][error]cannot find 'logger.level'." << std::endl;
            return false;
        }
        auto level_str = logger_config["level"].as<std::string>();
        auto level = spdlog::level::from_str(level_str);
        
        std::string pattern;
        if (logger_config["pattern"]) {
            pattern = logger_config["pattern"].as<std::string>();
        }
        else {
            pattern = "[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%^%l%$] [%s:%#] [%!] %v";
        }

        int flush_every;
        if (logger_config["flush_every_s"]) {
            flush_every = logger_config["flush_every_s"].as<int>();
        }
        else {
            flush_every = 1;
        }

        logger_ = spdlog::hourly_logger_mt("million_logger", log_file, 0, 0);
        logger_->set_level(level);
        logger_->set_pattern(pattern);

        spdlog::flush_every(std::chrono::seconds(flush_every));

        return true;
    }

    virtual void OnExit() override  {
        logger_->flush();
    }

    MILLION_MSG_DISPATCH(LoggerService);

    MILLION_MSG_HANDLE(LoggerLogMsg, msg) {
        auto level = static_cast<spdlog::level::level_enum>(msg->level);
        logger_->log(spdlog::source_loc{ msg->file, msg->line, msg->function }, level, msg->info);
        co_return;
    }

    MILLION_MSG_HANDLE(LoggerSetLevelMsg, msg) {
        auto level = spdlog::level::from_str(msg->level);
        logger_->set_level(level);
        co_return;
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};

Logger::Logger(Million* million) : million_(million) {}
Logger::~Logger() {

}

bool Logger::Init() {
    auto logger_opt = million_->NewService<LoggerService>();
    if (!logger_opt) {
        return false;
    }
    logger_handle_ = *logger_opt;
}

void Logger::Log(const ServiceHandle& sender, logger::LogLevel level, const char* file, int line, const char* function, const std::string& str) {
    million_->Send<LoggerLogMsg>(sender, logger_handle_, level, file, line, function, str);
}

void Logger::SetLevel(const ServiceHandle& sender, const std::string& level) {
    million_->Send<LoggerSetLevelMsg>(sender, logger_handle_, level);
}

} // namespace million