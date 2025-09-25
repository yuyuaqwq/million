#include <iostream>
#include <fstream>
#include <filesystem>

#include <yaml-cpp/yaml.h>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#include <spdlog/sinks/hourly_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <million/imillion.h>
#include <million/logger/api.h>

MILLION_MODULE_INIT();

namespace million {
namespace logger {

static_assert(static_cast<uint32_t>(Logger::LogLevel::kTrace) == spdlog::level::level_enum::trace);
static_assert(static_cast<uint32_t>(Logger::LogLevel::kDebug) == spdlog::level::level_enum::debug);
static_assert(static_cast<uint32_t>(Logger::LogLevel::kInfo) == spdlog::level::level_enum::info);
static_assert(static_cast<uint32_t>(Logger::LogLevel::kWarn) == spdlog::level::level_enum::warn);
static_assert(static_cast<uint32_t>(Logger::LogLevel::kError) == spdlog::level::level_enum::err);
static_assert(static_cast<uint32_t>(Logger::LogLevel::kCritical) == spdlog::level::level_enum::critical);
static_assert(static_cast<uint32_t>(Logger::LogLevel::kOff) == spdlog::level::level_enum::off);

class LoggerService : public IService {
    MILLION_SERVICE_DEFINE(LoggerService);

public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override {
        auto& settings = imillion().YamlSettings();

        logger().LOG_INFO("load 'logger' settings.");

        auto logger_settings = settings["logger"];
        if (!logger_settings) {
            logger().LOG_ERROR("cannot find 'logger'.");
            return false;
        }
        if (!logger_settings["log_file"]) {
            logger().LOG_ERROR("cannot find 'logger.log_file'.");
            return false;
        }
        auto log_file = logger_settings["log_file"].as<std::string>();

        if (!logger_settings["level"]) {
            logger().LOG_ERROR("cannot find 'logger.level'.");
            return false;
        }
        auto level_str = logger_settings["level"].as<std::string>();
        auto level = spdlog::level::from_str(level_str);
        
        std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [tid %t] [%^%l%$] [%s:%#] [%!] %v";
        if (logger_settings["pattern"]) {
            pattern = logger_settings["pattern"].as<std::string>();
        }

        int flush_every = 1;
        if (logger_settings["flush_every_s"]) {
            flush_every = logger_settings["flush_every_s"].as<int>();
        }

        auto console_level = spdlog::level::level_enum::off;;
        if (logger_settings["console_level"]) {
            auto level_str = logger_settings["console_level"].as<std::string>();
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

    virtual Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override {
        logger().BindService(service_handle());
        co_return nullptr;
    }

    virtual Task<MessagePointer> OnStop(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override {
        logger_->flush();
        co_return nullptr;
    }


    MILLION_MESSAGE_HANDLE(LoggerLog, msg) {
        auto level = static_cast<spdlog::level::level_enum>(msg->level);
        std::string short_func = logger().ExtractFunctionName(msg->source.function_name());
        logger_->log(spdlog::source_loc(msg->source.file_name(), msg->source.line(), short_func.c_str()), level, msg->msg);
        co_return nullptr;
    }

    MILLION_MESSAGE_HANDLE(LoggerLog2, msg) {
        auto level = static_cast<spdlog::level::level_enum>(msg->level);
        std::string short_func = logger().ExtractFunctionName(msg->source.function_name);
        logger_->log(spdlog::source_loc(msg->source.file_name.c_str(), msg->source.line, short_func.c_str()), level, msg->msg);
        co_return nullptr;
    }

    MILLION_MESSAGE_HANDLE(LoggerSetLevel, msg) {
        auto new_level = static_cast<spdlog::level::level_enum>(msg->new_level);
        logger_->set_level(new_level);
        co_return nullptr;
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};

extern "C" MILLION_LOGGER_API bool MillionModuleInit(IMillion* imillion) {
    auto& settings = imillion->YamlSettings();
    
    auto logger_service_opt = imillion->NewService<LoggerService>();
    if (!logger_service_opt) {
        return false;
    }

    return true;
}

} // namespace logger
} // namespace million