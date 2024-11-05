#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include "gateway_service.h"

namespace million {
namespace gateway {

MILLION_FUNC_API bool MillionModuleInit(IMillion* imillion) {
    auto& config = imillion->YamlConfig();
    auto handle = imillion->NewService<GatewayService>();
    return true;
}

} // namespace gateway
} // namespace million