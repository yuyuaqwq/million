#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <spdlog/spdlog.h>
// #include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/hourly_file_sink.h>

namespace million {
namespace logger {

MILLION_MSG_DEFINE(LoggerLogMsg, (std::string) info);

class LoggerService : public IService {
public:
    using Base = IService;
    using Base::Base;

    virtual void OnInit() override {
        auto logger = spdlog::hourly_logger_mt("million_logger", "logs/log.txt", 0, 0);
        logger->set_level(spdlog::level::info);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
        for (int i = 0; i < 1000; ++i) {
            logger->info("This is log message number {}", i);
        }

        // 确保日志刷新到文件
        logger->flush();

    }

    virtual Task OnMsg(MsgUnique msg) override {
        co_return;
    }

    MILLION_MSG_DISPATCH(LoggerService);

    MILLION_MSG_HANDLE(LoggerLogMsg, msg) {
        co_return;
    }

private:
};


MILLION_FUNC_EXPORT bool MillionModuleInit(million::IMillion* imillion) {
    auto& config = imillion->YamlConfig();
    auto handle = imillion->NewService<LoggerService>();

    return true;
}


} // namespace logger
} // namespace million