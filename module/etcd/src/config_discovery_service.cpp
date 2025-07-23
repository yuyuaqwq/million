// config_discovery_service.cpp
#include <config_discovery_service.h>

namespace million {
namespace example {

bool ConfigDiscoveryService::OnInit() {
    logger().LOG_INFO("ConfigDiscoveryService Init");

    // 获取 etcd 服务句柄
    auto handle = imillion().GetServiceByName(etcd::kEtcdServiceName);
    if (!handle) {
        logger().LOG_ERROR("Unable to find EtcdService");
        return false;
    }
    etcd_service_ = *handle;

    // 初始化 etcd 客户端
    if (!etcd::EtcdApi::Init(&imillion(), "http://127.0.0.1:2379")) {
        logger().LOG_ERROR("Failed to initialize etcd client");
        return false;
    }

    // 获取服务名称和地址
    service_name_ = "config-discovery-service";
    service_address_ = "127.0.0.1:8080";

    return true;
}

Task<MessagePointer> ConfigDiscoveryService::OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) {
    // 注册服务到 etcd
    bool registered = co_await etcd::EtcdApi::RegisterService(this, etcd_service_, service_name_, service_address_, 10);
    if (registered) {
        logger().LOG_INFO("Service {} registered at {}", service_name_, service_address_);
    } else {
        logger().LOG_ERROR("Failed to register service {}", service_name_);
    }

    // 加载配置
    auto config = co_await etcd::EtcdApi::LoadConfig(this, etcd_service_, "/config/myapp");
    if (config) {
        logger().LOG_INFO("Loaded config: {}", *config);
    } else {
        logger().LOG_WARN("No config found");
    }

    // 监听配置变化
    bool watching = co_await etcd::EtcdApi::WatchConfig(this, etcd_service_, "/config/myapp", 
        [this](const std::string& new_config) {
            OnConfigChange(new_config);
        });
    if (watching) {
        logger().LOG_INFO("Watching config changes");
    } else {
        logger().LOG_ERROR("Failed to watch config changes");
    }

    // 发现其他服务
    auto services = co_await etcd::EtcdApi::DiscoverService(this, etcd_service_, "other-service");
    logger().LOG_INFO("Discovered {} instances of other-service", services.size());
    for (const auto& addr : services) {
        logger().LOG_INFO("  - {}", addr);
    }

    co_return nullptr;
}

void ConfigDiscoveryService::OnConfigChange(const std::string& config_value) {
    logger().LOG_INFO("Config changed: {}", config_value);
    // 在这里处理配置变更
}

} // namespace example
} // namespace million