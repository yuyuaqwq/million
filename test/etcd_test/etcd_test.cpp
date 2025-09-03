#include <iostream>
#include <thread>
#include <chrono>

#include <million/imillion.h>
#include <yaml-cpp/yaml.h>

#include <etcd/etcd.h>
#include <etcd/config_discovery.h>

MILLION_MODULE_INIT();

namespace etcd = million::etcd;

// 测试消息定义
MILLION_MESSAGE_DEFINE_EMPTY(, TestCompleteMsg)

class EtcdTestService : public million::IService {
    MILLION_SERVICE_DEFINE(EtcdTestService);

public:
    using Base = million::IService;
    EtcdTestService(million::IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit() override {
        // 获取 EtcdService
        auto etcd_handle = imillion().FindServiceByName(etcd::kEtcdServiceName);
        if (!etcd_handle) {
            logger().LOG_ERROR("无法找到 EtcdService.");
            return false;
        }
        etcd_service_ = *etcd_handle;

        // 获取 ConfigDiscoveryService
        auto config_handle = imillion().FindServiceByName(etcd::kConfigDiscoveryServiceName);
        if (!config_handle) {
            logger().LOG_ERROR("无法找到 ConfigDiscoveryService.");
            return false;
        }
        config_discovery_service_ = *config_handle;

        return true;
    }

    virtual million::Task<million::MessagePointer> OnStart(::million::ServiceHandle sender, 
                                                          ::million::SessionId session_id, 
                                                          million::MessagePointer with_msg) override {
        logger().LOG_INFO("开始 ETCD 测试...");

        // 测试基础操作
        co_await TestBasicOperations();
        
        // 测试监听功能
        co_await TestWatchOperations();
        
        // 测试批量操作
        co_await TestBatchOperations();
        
        // 测试租约操作
        co_await TestLeaseOperations();
        
        // 测试配置发现
        co_await TestConfigDiscovery();
        
        // 测试服务注册发现
        co_await TestServiceDiscovery();

        logger().LOG_INFO("ETCD 测试完成!");
        
        co_return nullptr;
    }

private:
    // 测试基础的 Get/Put/Delete 操作
    million::Task<void> TestBasicOperations() {
        logger().LOG_INFO("开始测试基础操作...");

        // 测试 PUT 操作
        auto put_resp = co_await Call<etcd::EtcdPutReq, etcd::EtcdPutResp>(
            etcd_service_, "/test/key1", "test_value_1", 0);
        if (put_resp->success) {
            logger().LOG_INFO("PUT 操作成功: /test/key1 = test_value_1");
        } else {
            logger().LOG_ERROR("PUT 操作失败: {}", put_resp->error_message);
        }

        // 测试 GET 操作
        auto get_resp = co_await Call<etcd::EtcdGetReq, etcd::EtcdGetResp>(
            etcd_service_, "/test/key1");
        if (get_resp->success && get_resp->value) {
            logger().LOG_INFO("GET 操作成功: /test/key1 = {}", *get_resp->value);
        } else {
            logger().LOG_ERROR("GET 操作失败: {}", get_resp->error_message);
        }

        // 测试 DELETE 操作
        auto delete_resp = co_await Call<etcd::EtcdDeleteReq, etcd::EtcdDeleteResp>(
            etcd_service_, "/test/key1");
        if (delete_resp->success) {
            logger().LOG_INFO("DELETE 操作成功: /test/key1");
        } else {
            logger().LOG_ERROR("DELETE 操作失败: {}", delete_resp->error_message);
        }

        // 验证删除后的 GET
        auto get_after_delete = co_await Call<etcd::EtcdGetReq, etcd::EtcdGetResp>(
            etcd_service_, "/test/key1");
        if (!get_after_delete->success || !get_after_delete->value) {
            logger().LOG_INFO("删除验证成功: key 已不存在");
        } else {
            logger().LOG_ERROR("删除验证失败: key 仍然存在");
        }
    }

    // 测试监听操作
    million::Task<void> TestWatchOperations() {
        logger().LOG_INFO("开始测试监听操作...");

        // 开始监听
        auto watch_resp = co_await Call<etcd::EtcdWatchReq, etcd::EtcdWatchResp>(
            etcd_service_, "/test/watch_key", service_handle());
        if (watch_resp->success) {
            logger().LOG_INFO("监听启动成功: watch_id = {}", watch_resp->watch_id);
            current_watch_id_ = watch_resp->watch_id;

            // 等待一小段时间，确保监听已经开始
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // 修改被监听的 key
            auto put_resp = co_await Call<etcd::EtcdPutReq, etcd::EtcdPutResp>(
                etcd_service_, "/test/watch_key", "watched_value", 0);
            
            if (put_resp->success) {
                logger().LOG_INFO("监听测试: 已修改监听的 key");
            }

            // 等待监听事件
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // 停止监听
            auto unwatch_resp = co_await Call<etcd::EtcdUnwatchReq, etcd::EtcdUnwatchResp>(
                etcd_service_, current_watch_id_);
            if (unwatch_resp->success) {
                logger().LOG_INFO("监听停止成功");
            }
        } else {
            logger().LOG_ERROR("监听启动失败: {}", watch_resp->error_message);
        }
    }

    // 测试批量操作
    million::Task<void> TestBatchOperations() {
        logger().LOG_INFO("开始测试批量操作...");

        // 先放入一些测试数据
        co_await Call<etcd::EtcdPutReq, etcd::EtcdPutResp>(etcd_service_, "/test/batch1", "value1", 0);
        co_await Call<etcd::EtcdPutReq, etcd::EtcdPutResp>(etcd_service_, "/test/batch2", "value2", 0);
        co_await Call<etcd::EtcdPutReq, etcd::EtcdPutResp>(etcd_service_, "/test/batch3", "value3", 0);

        // 批量获取
        std::vector<std::string> keys = {"/test/batch1", "/test/batch2", "/test/batch3"};
        auto batch_resp = co_await Call<etcd::EtcdBatchGetReq, etcd::EtcdBatchGetResp>(
            etcd_service_, keys);
        
        if (batch_resp->success) {

            logger().LOG_INFO("批量获取成功，获取到 {} 个键值对", batch_resp->key_values.size());
            for (const auto& [key, value] : batch_resp->key_values) {
                logger().LOG_INFO("  {} = {}", key, value);
            }
        } else {
            logger().LOG_ERROR("批量获取失败: {}", batch_resp->error_message);
        }

        // 列出前缀匹配的 keys
        auto list_resp = co_await Call<etcd::EtcdListKeysReq, etcd::EtcdListKeysResp>(
            etcd_service_, "/test/batch");
        
        if (list_resp->success) {
            logger().LOG_INFO("前缀列举成功，找到 {} 个 key", list_resp->keys.size());
            for (const auto& key : list_resp->keys) {
                logger().LOG_INFO("  发现 key: {}", key);
            }
        } else {
            logger().LOG_ERROR("前缀列举失败: {}", list_resp->error_message);
        }
    }

    // 测试租约操作
    million::Task<void> TestLeaseOperations() {
        logger().LOG_INFO("开始测试租约操作...");

        // 创建租约
        auto lease_resp = co_await Call<etcd::EtcdLeaseGrantReq, etcd::EtcdLeaseGrantResp>(
            etcd_service_, 10); // 10秒TTL
        
        if (lease_resp->success) {
            logger().LOG_INFO("租约创建成功: lease_id = {}", lease_resp->lease_id);
            
            // 使用租约创建 key
            auto put_resp = co_await Call<etcd::EtcdPutReq, etcd::EtcdPutResp>(
                etcd_service_, "/test/lease_key", "lease_value", lease_resp->lease_id);
            
            if (put_resp->success) {
                logger().LOG_INFO("带租约的 key 创建成功");
                
                // 验证 key 存在
                auto get_resp = co_await Call<etcd::EtcdGetReq, etcd::EtcdGetResp>(
                    etcd_service_, "/test/lease_key");
                if (get_resp->success && get_resp->value) {
                    logger().LOG_INFO("租约 key 读取成功: {}", *get_resp->value);
                }
                
                // 撤销租约
                auto revoke_resp = co_await Call<etcd::EtcdLeaseRevokeReq, etcd::EtcdLeaseRevokeResp>(
                    etcd_service_, lease_resp->lease_id);
                
                if (revoke_resp->success) {
                    logger().LOG_INFO("租约撤销成功");
                    
                    // 等待一小段时间
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    
                    // 验证 key 已被删除
                    auto get_after_revoke = co_await Call<etcd::EtcdGetReq, etcd::EtcdGetResp>(
                        etcd_service_, "/test/lease_key");
                    if (!get_after_revoke->success || !get_after_revoke->value) {
                        logger().LOG_INFO("租约撤销验证成功: key 已被删除");
                    } else {
                        logger().LOG_ERROR("租约撤销验证失败: key 仍然存在");
                    }
                } else {
                    logger().LOG_ERROR("租约撤销失败: {}", revoke_resp->error_message);
                }
            }
        } else {
            logger().LOG_ERROR("租约创建失败: {}", lease_resp->error_message);
        }
    }

    // 测试配置发现
    million::Task<void> TestConfigDiscovery() {
        logger().LOG_INFO("开始测试配置发现...");

        // 设置配置
        auto set_resp = co_await Call<etcd::ConfigSetReq, etcd::ConfigSetResp>(
            config_discovery_service_, "database/host", "localhost");
        if (set_resp->success) {
            logger().LOG_INFO("配置设置成功: database/host = localhost");
        } else {
            logger().LOG_ERROR("配置设置失败: {}", set_resp->error_message);
        }

        // 获取配置
        auto get_resp = co_await Call<etcd::ConfigGetReq, etcd::ConfigGetResp>(
            config_discovery_service_, "database/host");
        if (get_resp->success && get_resp->config_value) {
            logger().LOG_INFO("配置获取成功: database/host = {}", *get_resp->config_value);
        } else {
            logger().LOG_ERROR("配置获取失败: {}", get_resp->error_message);
        }

        // 测试配置监听
        auto watch_resp = co_await Call<etcd::ConfigWatchReq, etcd::ConfigWatchResp>(
            config_discovery_service_, "database/host", service_handle());
        if (watch_resp->success) {
            logger().LOG_INFO("配置监听启动成功: watch_id = {}", watch_resp->watch_id);
            
            // 修改配置以触发监听事件
            auto update_resp = co_await Call<etcd::ConfigSetReq, etcd::ConfigSetResp>(
                config_discovery_service_, "database/host", "192.168.1.100");
            if (update_resp->success) {
                logger().LOG_INFO("配置更新成功，应该触发监听事件");
            }
            
            // 等待监听事件
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } else {
            logger().LOG_ERROR("配置监听启动失败: {}", watch_resp->error_message);
        }
    }

    // 测试服务注册发现
    million::Task<void> TestServiceDiscovery() {
        logger().LOG_INFO("开始测试服务注册发现...");

        // 注册服务
        std::vector<std::string> tags = {"web", "api", "v1.0"};
        auto register_resp = co_await Call<etcd::ServiceRegisterReq, etcd::ServiceRegisterResp>(
            config_discovery_service_, "user-service", "127.0.0.1", 8080, tags, 30);
        
        if (register_resp->success) {
            logger().LOG_INFO("服务注册成功: lease_id = {}", register_resp->lease_id);
            current_lease_id_ = register_resp->lease_id;
            
            // 发现服务
            auto discover_resp = co_await Call<etcd::ServiceDiscoverReq, etcd::ServiceDiscoverResp>(
                config_discovery_service_, "user-service");
            
            if (discover_resp->success) {
                logger().LOG_INFO("服务发现成功，找到 {} 个服务实例", discover_resp->service_endpoints.size());
                for (const auto& endpoint : discover_resp->service_endpoints) {
                    logger().LOG_INFO("  服务端点: {}", endpoint);
                }
            } else {
                logger().LOG_ERROR("服务发现失败: {}", discover_resp->error_message);
            }
            
            // 发送心跳
            auto heartbeat_resp = co_await Call<etcd::ServiceHeartbeatReq, etcd::ServiceHeartbeatResp>(
                config_discovery_service_, current_lease_id_);
            
            if (heartbeat_resp->success) {
                logger().LOG_INFO("服务心跳成功");
            } else {
                logger().LOG_ERROR("服务心跳失败: {}", heartbeat_resp->error_message);
            }
            
            // 注销服务
            auto unregister_resp = co_await Call<etcd::ServiceUnregisterReq, etcd::ServiceUnregisterResp>(
                config_discovery_service_, current_lease_id_);
            
            if (unregister_resp->success) {
                logger().LOG_INFO("服务注销成功");
            } else {
                logger().LOG_ERROR("服务注销失败: {}", unregister_resp->error_message);
            }
        } else {
            logger().LOG_ERROR("服务注册失败: {}", register_resp->error_message);
        }
    }

public:
    // 处理监听事件
    MILLION_MESSAGE_HANDLE(etcd::EtcdWatchEvent, msg) {
        logger().LOG_INFO("收到 ETCD 监听事件: key={}, value={}, type={}, watch_id={}", 
                         msg->key, msg->value, msg->event_type, msg->watch_id);
        co_return nullptr;
    }

    // 处理配置变更事件
    MILLION_MESSAGE_HANDLE(etcd::ConfigChangeEvent, msg) {
        logger().LOG_INFO("收到配置变更事件: path={}, old={}, new={}, type={}, watch_id={}", 
                         msg->config_path, msg->old_value, msg->new_value, msg->change_type, msg->watch_id);
        co_return nullptr;
    }

private:
    million::ServiceHandle etcd_service_;
    million::ServiceHandle config_discovery_service_;
    uint64_t current_watch_id_ = 0;
    int64_t current_lease_id_ = 0;
};

class EtcdTestApp : public million::IMillion {
    // 使用默认构造函数
};

int main() {
    auto test_app = std::make_unique<EtcdTestApp>();
    if (!test_app->Init("etcd_test_settings.yaml")) {
        std::cerr << "初始化测试应用失败" << std::endl;
        return 1;
    }
    test_app->Start();

    auto service_opt = test_app->NewService<EtcdTestService>();
    if (!service_opt) {
        std::cerr << "创建测试服务失败" << std::endl;
        return 1;
    }
    auto service_handle = *service_opt;

    std::cout << "ETCD 测试开始运行..." << std::endl;
    
    // 运行 30 秒以完成所有测试
    std::this_thread::sleep_for(std::chrono::seconds(30));

    std::cout << "ETCD 测试运行结束" << std::endl;
    return 0;
}

