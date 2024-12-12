#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <agent/api.h>

#include "agent_service.h"

MILLION_MODULE_INIT();

namespace million {
namespace agent {

extern "C" AGENT_FUNC_API bool MillionModuleInit(IMillion* imillion) {
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