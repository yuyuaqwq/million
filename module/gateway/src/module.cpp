#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <gateway/api.h>

#include "gateway_service.h"

MILLION_MODULE_INIT();

namespace million {
namespace gateway {

extern "C" MILLION_GATEWAY_API bool MillionModuleInit(IMillion* imillion) {
    auto& settings = imillion->YamlSettings();
    auto handle = imillion->NewService<GatewayService>();
    if (!handle) {
        return false;
    }
    imillion->SetServiceName(*handle, "GatewayService");

    return true;
}

} // namespace gateway
} // namespace million