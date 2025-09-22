#include "config_discovery_service.h"

#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <sstream>

#include <etcd/ss_etcd.pb.h>

namespace million {
namespace etcd {

bool ConfigDiscoveryService::OnInit() {
    logger().LOG_INFO("ConfigDiscoveryService Init");

    // 设置服务名
    imillion().SetServiceNameId(service_handle(), module::module_id, ss::ServiceNameId_descriptor(), ss::SERVICE_NAME_ID_CONFIG_DISCOVERY);

    // 获取EtcdService
    auto etcd_service_opt = imillion().FindServiceByNameId(module::module_id, ss::ServiceNameId_descriptor(), ss::SERVICE_NAME_ID_ETCD);
    if (!etcd_service_opt) {
        logger().LOG_ERROR("Unable to find EtcdService.");
        return false;
    }
    etcd_service_ = *etcd_service_opt;

    // 读取配置前缀
    const auto& settings = imillion().YamlSettings();
    const auto& etcd_settings = settings["etcd"];
    if (etcd_settings) {
        const auto& config_prefix = etcd_settings["config_prefix"];
        if (config_prefix) {
            config_prefix_ = config_prefix.as<std::string>();
        }
        
        const auto& service_prefix = etcd_settings["service_prefix"];
        if (service_prefix) {
            service_prefix_ = service_prefix.as<std::string>();
        }
    }

    logger().LOG_INFO("ConfigDiscoveryService initialized with config_prefix={}, service_prefix={}", 
                     config_prefix_, service_prefix_);

    return true;
}

std::string ConfigDiscoveryService::BuildConfigPath(const std::string& config_path) {
    return config_prefix_ + config_path;
}

std::string ConfigDiscoveryService::BuildServicePath(const std::string& service_name, const std::string& instance_id) {
    return service_prefix_ + service_name + "/" + instance_id;
}

std::string ConfigDiscoveryService::BuildServiceInstanceKey(const std::string& service_name, const std::string& address, int32_t port) {
    return address + ":" + std::to_string(port);
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, ConfigGetReq, msg) {
    Send<EtcdGetReq>(etcd_service_, BuildConfigPath(msg->config_path));
    co_return nullptr;
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, ConfigSetReq, msg) {
    // 配置通常不需要过期
    Send<EtcdPutReq>(etcd_service_, BuildConfigPath(msg->config_path), std::move(msg->config_value), 0);
    co_return nullptr;
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, ConfigWatchReq, msg) {
    uint64_t watch_id = next_watch_id_++;
    config_watch_callbacks_[watch_id] = msg->callback_service;
    config_watch_paths_[watch_id] = msg->config_path;
    
    // 设置为当前服务处理回调
    Send<EtcdWatchReq>(etcd_service_, BuildConfigPath(msg->config_path), service_handle());
    
    // 直接返回成功（实际的watch结果会在EtcdWatchResp中处理）
    logger().LOG_DEBUG("ConfigWatch config_path={}, watch_id={}", msg->config_path, watch_id);
    co_return make_message<ConfigWatchResp>(true, watch_id, "");
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, ServiceRegisterReq, msg) {
    // 构建服务实例的键值
    std::string instance_id = BuildServiceInstanceKey(msg->service_name, msg->service_address, msg->service_port);
    std::string service_key = BuildServicePath(msg->service_name, instance_id);
    
    // 构建服务信息的值
    std::ostringstream service_info;
    service_info << "{";
    service_info << "\"address\":\"" << msg->service_address << "\",";
    service_info << "\"port\":" << msg->service_port << ",";
    service_info << "\"tags\":[";
    for (size_t i = 0; i < msg->tags.size(); ++i) {
        service_info << "\"" << msg->tags[i] << "\"";
        if (i < msg->tags.size() - 1) {
            service_info << ",";
        }
    }
    service_info << "]}";
    
    // 如果需要TTL，先申请租约
    if (msg->ttl > 0) {
        Send<EtcdLeaseGrantReq>(etcd_service_, msg->ttl);
        // 租约申请的结果会在EtcdLeaseGrantResp中处理
    } else {
        // 不需要TTL，直接注册
        Send<EtcdPutReq>(etcd_service_, service_key, service_info.str(), 0);
    }
    co_return nullptr;
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, ServiceDiscoverReq, msg) {
    Send<EtcdListKeysReq>(etcd_service_, BuildServicePath(msg->service_name, ""));
    co_return nullptr;
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, ServiceUnregisterReq, msg) {
    Send<EtcdLeaseRevokeReq>(etcd_service_, msg->lease_id);
    co_return nullptr;
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, ServiceHeartbeatReq, msg) {
    // 这里可以实现租约续期功能
    // etcd-cpp-apiv3库中可能需要使用leasekeepalive功能
    logger().LOG_DEBUG("ServiceHeartbeat lease_id={}", msg->lease_id);
    co_return make_message<ServiceHeartbeatResp>(true, "");
}

// 处理EtcdService的回调
MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, EtcdGetResp, msg) {
    co_return make_message<ConfigGetResp>(msg->success, std::move(msg->value), std::move(msg->error_message));
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, EtcdPutResp, msg) {
    co_return make_message<ConfigSetResp>(msg->success, std::move(msg->error_message));
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, EtcdDeleteResp, msg) {
    // 可以用于处理服务注销等操作的响应
    logger().LOG_DEBUG("EtcdDeleteResp: success={}, error={}", msg->success, msg->error_message);
    co_return nullptr;
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, EtcdLeaseGrantResp, msg) {
    if (msg->success) {
        co_return make_message<ServiceRegisterResp>(true, std::move(msg->lease_id), "");
    } else {
        co_return make_message<ServiceRegisterResp>(false, 0, std::move(msg->error_message));
    }
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, EtcdLeaseRevokeResp, msg) {
    co_return make_message<ServiceUnregisterResp>(msg->success, std::move(msg->error_message));
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, EtcdListKeysResp, msg) {
    std::vector<std::string> service_endpoints;
    if (msg->success) {
        // 解析服务端点信息
        for (const auto& key : msg->keys) {
            // 从键中提取服务端点信息
            // 键格式: /services/service_name/address:port
            size_t last_slash = key.find_last_of('/');
            if (last_slash != std::string::npos) {
                std::string endpoint = key.substr(last_slash + 1);
                service_endpoints.push_back(endpoint);
            }
        }
    }
    
    logger().LOG_DEBUG("ServiceDiscover found {} endpoints", service_endpoints.size());
    co_return make_message<ServiceDiscoverResp>(msg->success, std::move(service_endpoints), std::move(msg->error_message));
}

} // namespace etcd
} // namespace million 