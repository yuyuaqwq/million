#include "config_discovery_service.h"
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <sstream>

namespace million {
namespace etcd {

bool ConfigDiscoveryService::OnInit() {
    logger().LOG_INFO("ConfigDiscoveryService Init");

    // 设置服务名
    imillion().SetServiceName(service_handle(), kConfigDiscoveryServiceName);

    // 获取EtcdService
    auto etcd_service_opt = imillion().GetServiceByName(kEtcdServiceName);
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
    auto etcd_req = std::make_unique<EtcdGetReq>();
    etcd_req->key = BuildConfigPath(msg->config_path);
    
    Send(etcd_service_, std::move(etcd_req));
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, ConfigSetReq, msg) {
    auto etcd_req = std::make_unique<EtcdPutReq>();
    etcd_req->key = BuildConfigPath(msg->config_path);
    etcd_req->value = msg->config_value;
    etcd_req->lease_id = 0; // 配置通常不需要过期
    
    Send(etcd_service_, std::move(etcd_req));
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, ConfigWatchReq, msg) {
    uint64_t watch_id = next_watch_id_++;
    config_watch_callbacks_[watch_id] = msg->callback_service;
    config_watch_paths_[watch_id] = msg->config_path;
    
    auto etcd_req = std::make_unique<EtcdWatchReq>();
    etcd_req->key = BuildConfigPath(msg->config_path);
    etcd_req->callback_service = service_handle(); // 设置为当前服务处理回调
    
    Send(etcd_service_, std::move(etcd_req));
    
    // 直接返回成功（实际的watch结果会在EtcdWatchResp中处理）
    auto reply = std::make_unique<ConfigWatchResp>();
    reply->success = true;
    reply->watch_id = watch_id;
    reply->error_message = "";
    
    logger().LOG_DEBUG("ConfigWatch config_path={}, watch_id={}", msg->config_path, watch_id);
    Reply(sender, session_id, std::move(reply));
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
        auto lease_req = std::make_unique<EtcdLeaseGrantReq>();
        lease_req->ttl = msg->ttl;
        Send(etcd_service_, std::move(lease_req));
        // 租约申请的结果会在EtcdLeaseGrantResp中处理
    } else {
        // 不需要TTL，直接注册
        auto etcd_req = std::make_unique<EtcdPutReq>();
        etcd_req->key = service_key;
        etcd_req->value = service_info.str();
        etcd_req->lease_id = 0;
        
        Send(etcd_service_, std::move(etcd_req));
    }
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, ServiceDiscoverReq, msg) {
    auto etcd_req = std::make_unique<EtcdListKeysReq>();
    etcd_req->prefix = BuildServicePath(msg->service_name, "");
    
    session().Call(etcd_service_, std::move(etcd_req));
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, ServiceUnregisterReq, msg) {
    auto etcd_req = std::make_unique<EtcdLeaseRevokeReq>();
    etcd_req->lease_id = msg->lease_id;
    
    session().Call(etcd_service_, std::move(etcd_req));
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, ServiceHeartbeatReq, msg) {
    // 这里可以实现租约续期功能
    // etcd-cpp-apiv3库中可能需要使用leasekeepalive功能
    auto reply = std::make_unique<ServiceHeartbeatResp>();
    reply->success = true;
    reply->error_message = "";
    
    logger().LOG_DEBUG("ServiceHeartbeat lease_id={}", msg->lease_id);
    session().Send(std::move(reply));
}

// 处理EtcdService的回调
MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, EtcdGetResp, msg) {
    auto reply = std::make_unique<ConfigGetResp>();
    reply->success = msg->success;
    reply->config_value = msg->value;
    reply->error_message = msg->error_message;
    
    session().Send(std::move(reply));
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, EtcdPutResp, msg) {
    auto reply = std::make_unique<ConfigSetResp>();
    reply->success = msg->success;
    reply->error_message = msg->error_message;
    
    session().Send(std::move(reply));
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, EtcdDeleteResp, msg) {
    // 可以用于处理服务注销等操作的响应
    logger().LOG_DEBUG("EtcdDeleteResp: success={}, error={}", msg->success, msg->error_message);
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, EtcdLeaseGrantResp, msg) {
    if (msg->success) {
        auto reply = std::make_unique<ServiceRegisterResp>();
        reply->success = true;
        reply->lease_id = msg->lease_id;
        reply->error_message = "";
        
        session().Send(std::move(reply));
    } else {
        auto reply = std::make_unique<ServiceRegisterResp>();
        reply->success = false;
        reply->lease_id = 0;
        reply->error_message = msg->error_message;
        
        session().Send(std::move(reply));
    }
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, EtcdLeaseRevokeResp, msg) {
    auto reply = std::make_unique<ServiceUnregisterResp>();
    reply->success = msg->success;
    reply->error_message = msg->error_message;
    
    session().Send(std::move(reply));
}

MILLION_MESSAGE_HANDLE_IMPL(ConfigDiscoveryService, EtcdListKeysResp, msg) {
    auto reply = std::make_unique<ServiceDiscoverResp>();
    reply->success = msg->success;
    reply->error_message = msg->error_message;
    
    if (msg->success) {
        // 解析服务端点信息
        for (const auto& key : msg->keys) {
            // 从键中提取服务端点信息
            // 键格式: /services/service_name/address:port
            size_t last_slash = key.find_last_of('/');
            if (last_slash != std::string::npos) {
                std::string endpoint = key.substr(last_slash + 1);
                reply->service_endpoints.push_back(endpoint);
            }
        }
    }
    
    logger().LOG_DEBUG("ServiceDiscover found {} endpoints", reply->service_endpoints.size());
    session().Send(std::move(reply));
}

} // namespace etcd
} // namespace million 