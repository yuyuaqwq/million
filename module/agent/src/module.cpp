#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include "agent_service.h"

namespace million {
namespace agent {

MILLION_FUNC_API bool MillionModuleInit(IMillion* imillion) {
    AgentLogicHandler::Instance().ExecInitLogicQueue();

    auto handle = imillion->NewService<NodeMgrService>();
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

} // namespace agent
} // namespace million