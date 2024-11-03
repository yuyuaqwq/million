#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

namespace million {
namespace logger {

MILLION_MSG_DEFINE(LoggerLogMsg, (std::string) info);

class LoggerService : public IService {

    MILLION_MSG_DISPATCH(LoggerLogMsg);

    MILLION_MSG_HANDLE(LoggerLogMsg, msg) {

    }

private:
};


MILLION_FUNC_EXPORT bool MillionModuleInit(million::IMillion* imillion) {
    auto& config = imillion->YamlConfig();

    return true;
}


} // namespace logger
} // namespace million