#include <million/imillion.h>
#include <million/tlog/tlog_service.h>
#include <million/tlog/api.h>

MILLION_MODULE_INIT();

namespace million {
namespace tlog {

extern "C" MILLION_TLOG_API bool MillionModuleInit(IMillion* imillion) {
    auto& settings = imillion->YamlSettings();

    auto tlog_service_opt = imillion->NewService<TLogService>();
    if (!tlog_service_opt) {
        return false;
    }

    logger().LOG_INFO("TLog module initialized successfully");
    return true;
}

} // namespace tlog
} // namespace million
