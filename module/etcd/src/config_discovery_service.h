// config_discovery_service.h
#pragma once

#include <million/imillion.h>
#include <etcd/api.h>

namespace million {
namespace example {

// 配置管理服务
class ConfigDiscoveryService : public IService {
    MILLION_SERVICE_DEFINE(ConfigDiscoveryService);

public:
    using Base = IService;
    ConfigDiscoveryService(IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit() override;
    virtual Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override;

    // 配置变更回调
    void OnConfigChange(const std::string& config_value);

private:
    ServiceHandle etcd_service_;
    std::string service_name_;
    std::string service_address_;
};

} // namespace example
} // namespace million