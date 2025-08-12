#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <million/imillion.h>

#include <etcd/config_discovery.h>
#include <etcd/etcd.h>

namespace million {
namespace etcd {

class ConfigDiscoveryService : public IService {
    MILLION_SERVICE_DEFINE(ConfigDiscoveryService);

public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override;

private:
    ServiceHandle etcd_service_;
    std::string config_prefix_ = "/config/";
    std::string service_prefix_ = "/services/";
    
    // 监听管理
    uint64_t next_watch_id_ = 1;
    std::unordered_map<uint64_t, ServiceHandle> config_watch_callbacks_;
    std::unordered_map<uint64_t, std::string> config_watch_paths_;

public:
    // 配置操作消息处理
    MILLION_MESSAGE_HANDLE(ConfigGetReq, msg);
    MILLION_MESSAGE_HANDLE(ConfigSetReq, msg);
    MILLION_MESSAGE_HANDLE(ConfigWatchReq, msg);
    
    // 服务注册和发现消息处理
    MILLION_MESSAGE_HANDLE(ServiceRegisterReq, msg);
    MILLION_MESSAGE_HANDLE(ServiceDiscoverReq, msg);
    MILLION_MESSAGE_HANDLE(ServiceUnregisterReq, msg);
    MILLION_MESSAGE_HANDLE(ServiceHeartbeatReq, msg);

private:
    std::string BuildConfigPath(const std::string& config_path);
    std::string BuildServicePath(const std::string& service_name, const std::string& instance_id);
    std::string BuildServiceInstanceKey(const std::string& service_name, const std::string& address, int32_t port);
    
    // 处理EtcdService的回调
    MILLION_MESSAGE_HANDLE(EtcdGetResp, msg);
    MILLION_MESSAGE_HANDLE(EtcdPutResp, msg);
    MILLION_MESSAGE_HANDLE(EtcdDeleteResp, msg);
    MILLION_MESSAGE_HANDLE(EtcdLeaseGrantResp, msg);
    MILLION_MESSAGE_HANDLE(EtcdLeaseRevokeResp, msg);
    MILLION_MESSAGE_HANDLE(EtcdListKeysResp, msg);
};

} // namespace etcd
} // namespace million 