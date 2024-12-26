#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <agent/api.h>

#include "agent_service.h"

MILLION_MODULE_INIT();

namespace million {
namespace agent {

extern "C" MILLION_AGENT_API bool MillionModuleInit(IMillion* imillion) {
    auto handle = imillion->NewService<NodeMgrService>();
    handle = imillion->NewService<AgentMgrService>();
    return true;
}

} // namespace agent
} // namespace million