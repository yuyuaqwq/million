#include <iostream>

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>
#include <million/etcd/api.h>

#include "etcd_service.h"
#include "config_discovery_service.h"

MILLION_MODULE_INIT();

namespace million {
namespace etcd {

extern "C" MILLION_ETCD_API bool MillionModuleInit(IMillion* imillion) {
    auto& settings = imillion->YamlSettings();

    // 创建EtcdService
    auto etcd_service_opt = imillion->NewService<EtcdService>();
    if (!etcd_service_opt) {
        return false;
    }

    // 创建ConfigDiscoveryService
    auto config_discovery_service_opt = imillion->NewService<ConfigDiscoveryService>();
    if (!config_discovery_service_opt) {
        return false;
    }

    return true;
}

} // namespace etcd
} // namespace million 