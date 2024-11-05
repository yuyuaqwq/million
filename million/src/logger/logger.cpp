#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#include <spdlog/sinks/hourly_file_sink.h>

#include <million/logger/logger.h>
#include <million/imillion.h>

#include "logger.h"

namespace million {
namespace logger {

MILLION_OBJECT_API ServiceHandle logger_handle;

static_assert(LogLevel::kTrace == spdlog::level::level_enum::trace);
static_assert(LogLevel::kDebug == spdlog::level::level_enum::debug);
static_assert(LogLevel::kInfo == spdlog::level::level_enum::info);
static_assert(LogLevel::kWarn == spdlog::level::level_enum::warn);
static_assert(LogLevel::kErr == spdlog::level::level_enum::err);
static_assert(LogLevel::kCritical == spdlog::level::level_enum::critical);
static_assert(LogLevel::kOff == spdlog::level::level_enum::off);

class LoggerService : public IService {
public:
    using Base = IService;
    using Base::Base;

    virtual void OnInit() override {
        auto& config = imillion_->YamlConfig();
        auto logger_config = config["logger"];
        if (!logger_config) {
            throw ConfigException("cannot find 'logger'.");
        }
        if (!logger_config["log_file"]) {
            throw ConfigException("cannot find 'logger.log_file'.");
        }
        auto log_file = logger_config["log_file"].as<std::string>();

        if (!logger_config["level"]) {
            throw ConfigException("cannot find 'logger.level'.");
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
    }

    virtual void OnExit() override  {
        logger_->flush();
    }

    virtual Task OnMsg(MsgUnique msg) override {
        co_await MsgDispatch(std::move(msg));
        co_return;
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

bool LoggerInit(million::IMillion* imillion) {
    logger_handle = imillion->NewService<LoggerService>();
    return true;
}


} // namespace logger
} // namespace million