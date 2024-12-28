#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#include <spdlog/sinks/hourly_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <million/logger.h>

#include "million.h"

namespace million {

static_assert(static_cast<uint32_t>(ss::logger::LOG_LEVEL_TRACE) == spdlog::level::level_enum::trace);
static_assert(static_cast<uint32_t>(ss::logger::LOG_LEVEL_DEBUG) == spdlog::level::level_enum::debug);
static_assert(static_cast<uint32_t>(ss::logger::LOG_LEVEL_INFO) == spdlog::level::level_enum::info);
static_assert(static_cast<uint32_t>(ss::logger::LOG_LEVEL_WARN) == spdlog::level::level_enum::warn);
static_assert(static_cast<uint32_t>(ss::logger::LOG_LEVEL_ERR) == spdlog::level::level_enum::err);
static_assert(static_cast<uint32_t>(ss::logger::LOG_LEVEL_CRITICAL) == spdlog::level::level_enum::critical);
static_assert(static_cast<uint32_t>(ss::logger::LOG_LEVEL_OFF) == spdlog::level::level_enum::off);

class LoggerService : public IService {
public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit(MsgUnique init_msg) override {
        auto& config = imillion().YamlConfig();

        std::cout << "[logger] [info] load 'logger' config." << std::endl;

        auto logger_config = config["logger"];
        if (!logger_config) {
            std::cerr << "[logger] [error] cannot find 'logger'." << std::endl;
            return false;
        }
        if (!logger_config["log_file"]) {
            std::cerr << "[logger] [error] cannot find 'logger.log_file'." << std::endl;
            return false;
        }
        auto log_file = logger_config["log_file"].as<std::string>();

        if (!logger_config["level"]) {
            std::cerr << "[logger] [error] cannot find 'logger.level'." << std::endl;
            return false;
        }
        auto level_str = logger_config["level"].as<std::string>();
        auto level = spdlog::level::from_str(level_str);
        
        std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [tid %t] [%^%l%$] [%s:%#] [%!] %v";
        if (logger_config["pattern"]) {
            pattern = logger_config["pattern"].as<std::string>();
        }

        int flush_every = 1;
        if (logger_config["flush_every_s"]) {
            flush_every = logger_config["flush_every_s"].as<int>();
        }

        auto console_level = spdlog::level::level_enum::off;;
        if (logger_config["console_level"]) {
            auto level_str = logger_config["console_level"].as<std::string>();
            console_level = spdlog::level::from_str(level_str);;
        }

        logger_ = spdlog::hourly_logger_st("logger", log_file, 0, 0);
        logger_->set_level(level);
        logger_->set_pattern(pattern);

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
        console_sink->set_level(console_level);
        console_sink->set_pattern(pattern);

        logger_->sinks().push_back(console_sink);

        spdlog::flush_every(std::chrono::seconds(flush_every));

        return true;
    }

    virtual void OnStop(ServiceHandle sender, SessionId session_id) override  {
        logger_->flush();
    }

    MILLION_MSG_DISPATCH(LoggerService);

    using Log = ss::logger::Log;
    MILLION_MSG_HANDLE(Log, msg) {
        auto level = static_cast<spdlog::level::level_enum>(msg->level());
        logger_->log(spdlog::source_loc(msg->file().c_str(), msg->line(), msg->function().c_str()), level, msg->msg());
        co_return;
    }

    using SetLevel = ss::logger::SetLevel;
    MILLION_MSG_HANDLE(SetLevel, msg) {
        auto level = static_cast<spdlog::level::level_enum>(msg->level());
        logger_->set_level(level);
        co_return;
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};

Logger::Logger(Million* million) : million_(million) {}

Logger::~Logger() = default;

bool Logger::Init() {
    auto logger_opt = million_->imillion().NewService<LoggerService>();
    if (!logger_opt) {
        return false;
    }
    logger_handle_ = ServiceHandle(*logger_opt);
}

void Logger::Log(const ServiceHandle& sender, const std::source_location& source, ss::logger::LogLevel level, const std::string& msg) {
    million_->imillion().Send<ss::logger::Log>(sender, logger_handle_, level, source.file_name(), source.function_name(), source.line(), msg);
}

void Logger::SetLevel(const ServiceHandle& sender, ss::logger::LogLevel level) {
    million_->imillion().Send<ss::logger::SetLevel>(sender, logger_handle_, level);
}

} // namespace million