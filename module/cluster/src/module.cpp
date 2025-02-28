#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <cluster/api.h>

#include "cluster_service.h"

MILLION_MODULE_INIT();

namespace million {
namespace cluster {

extern "C" MILLION_CLUSTER_API bool MillionModuleInit(IMillion* imillion) {
    auto& settings = imillion->YamlSettings();
    auto handle = imillion->NewService<ClusterService>();
    if (!handle) {
        return false;
    }
    imillion->SetServiceName(*handle, "ClusterService");
    return true;
}

} // namespace cluster
} // namespace million