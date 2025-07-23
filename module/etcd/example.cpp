// 示例：如何在您的服务中使用 etcd 模块

#include <million/imillion.h>
#include <etcd/api.h>

class MyService : public IService {
    MILLION_SERVICE_DEFINE(MyService);

public:
    using Base = IService;
    MyService(IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit() override {
        // 获取 etcd 服务句柄
        auto handle = imillion().GetServiceByName(million::etcd::kEtcdServiceName);
        if (handle) {
            etcd_service_ = *handle;
        } else {
            logger().LOG_ERROR("Unable to find EtcdService");
            return false;
        }

        // 初始化 etcd 客户端
        if (!million::etcd::EtcdApi::Init(&imillion(), "http://127.0.0.1:2379")) {
            logger().LOG_ERROR("Failed to initialize etcd client");
            return false;
        }

        return true;
    }

    // 示例：基本键值操作
    Task<void> KeyValueExample() {
        // 设置键值对
        bool success = co_await million::etcd::EtcdApi::Set(this, etcd_service_, "mykey", "myvalue");
        if (success) {
            logger().LOG_INFO("Successfully set key 'mykey'");
        } else {
            logger().LOG_ERROR("Failed to set key 'mykey'");
        }

        // 获取键值
        auto value = co_await million::etcd::EtcdApi::Get(this, etcd_service_, "mykey");
        if (value) {
            logger().LOG_INFO("Get key 'mykey' with value '{}'", *value);
        } else {
            logger().LOG_WARN("Key 'mykey' not found");
        }

        // 删除键
        success = co_await million::etcd::EtcdApi::Del(this, etcd_service_, "mykey");
        if (success) {
            logger().LOG_INFO("Successfully deleted key 'mykey'");
        } else {
            logger().LOG_ERROR("Failed to delete key 'mykey'");
        }
    }

    // 示例：配置管理
    Task<void> ConfigExample() {
        // 加载配置
        auto config = co_await million::etcd::EtcdApi::LoadConfig(this, etcd_service_, "/config/myapp");
        if (config) {
            logger().LOG_INFO("Loaded config: {}", *config);
        } else {
            logger().LOG_WARN("No config found");
        }

        // 监听配置变化
        bool watching = co_await million::etcd::EtcdApi::WatchConfig(this, etcd_service_, "/config/myapp",
            [this](const std::string& new_config) {
                logger().LOG_INFO("Config changed: {}", new_config);
                // 处理配置变更
            });
        if (watching) {
            logger().LOG_INFO("Watching config changes");
        } else {
            logger().LOG_ERROR("Failed to watch config changes");
        }
    }

    // 示例：服务注册与发现
    Task<void> ServiceDiscoveryExample() {
        std::string service_name = "my-service";
        std::string service_address = "127.0.0.1:8080";

        // 注册服务
        bool registered = co_await million::etcd::EtcdApi::RegisterService(this, etcd_service_, 
            service_name, service_address, 10);
        if (registered) {
            logger().LOG_INFO("Service {} registered at {}", service_name, service_address);
        } else {
            logger().LOG_ERROR("Failed to register service {}", service_name);
        }

        // 发现服务
        auto services = co_await million::etcd::EtcdApi::DiscoverService(this, etcd_service_, "other-service");
        logger().LOG_INFO("Discovered {} instances of other-service", services.size());
        for (const auto& addr : services) {
            logger().LOG_INFO("  - {}", addr);
        }

        // 注销服务
        bool unregistered = co_await million::etcd::EtcdApi::UnregisterService(this, etcd_service_, 
            service_name, service_address);
        if (unregistered) {
            logger().LOG_INFO("Service {} unregistered", service_name);
        } else {
            logger().LOG_ERROR("Failed to unregister service {}", service_name);
        }
    }

private:
    ServiceHandle etcd_service_;
};