#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include "gateway_service.h"

namespace million {
namespace gateway {

MILLION_FUNC_API bool MillionModuleInit(IMillion* imillion) {
    AgentLogicHandler::Instance().ExecInitLogicQueue();

    auto& config = imillion->YamlConfig();
    auto handle = imillion->NewService<GatewayService>();
    if (!handle) {
        return false;
    }
    imillion->SetServiceName(*handle, "GatewayService");

    handle = imillion->NewService<NodeMgrService>();
    if (!handle) {
        return false;
    }
    imillion->SetServiceName(*handle, "NodeMgrService");

    handle = imillion->NewService<AgentMgrService>();
    if (!handle) {
        return false;
    }
    imillion->SetServiceName(*handle, "AgentMgrService");

    return true;
}

} // namespace gateway
} // namespace million