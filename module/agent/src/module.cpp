#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <agent/api.h>

#include <agent/ss_agent.pb.h>

#include "agent_mgr_service.h"

MILLION_MODULE_INIT();

namespace million {
namespace agent {

extern "C" MILLION_AGENT_API bool MillionModuleInit(IMillion* imillion) {
    auto handle = imillion->NewServiceWithoutStart<AgentMgrService>();
    return true;
}

extern "C" MILLION_AGENT_API void MillionModuleStart(IMillion* imillion) {
    auto handle = imillion->FindServiceByNameId(module::module_id, ss::ServiceNameId_descriptor(), ss::SERVICE_NAME_ID_AGENT_MANAGER);
    if (handle) {
        imillion->StartService(*handle, nullptr);
    }
    return;
}



} // namespace agent
} // namespace million