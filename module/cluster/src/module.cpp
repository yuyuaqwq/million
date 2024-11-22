#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include "cluster_service.h"

namespace million {
namespace cluster {

MILLION_FUNC_API bool MillionModuleInit(IMillion* imillion) {
    auto& config = imillion->YamlConfig();
    auto handle = imillion->NewService<ClusterService>();
    if (!handle) {
        return false;
    }
    imillion->SetServiceUniqueName(*handle, "ClusterService");
    return true;
}

} // namespace cluster
} // namespace million