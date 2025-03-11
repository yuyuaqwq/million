#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <agent/api.h>

#include "agent_mgr_service.h"

MILLION_MODULE_INIT();

namespace million {
namespace agent {

extern "C" MILLION_AGENT_API bool MillionModuleInit(IMillion* imillion) {
    auto handle = imillion->NewServiceWithoutStart<AgentMgrService>();
    return true;
}

extern "C" MILLION_AGENT_API void MillionModuleStart(IMillion* imillion) {
    auto handle = imillion->GetServiceByName("AgentMgrService");
    if (handle) {
        imillion->StartService(*handle);
    }
    return;
}



} // namespace agent
} // namespace million