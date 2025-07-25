#pragma once

#include <memory>
#include <string>
#include <map>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include <million/imillion.h>
#include <asio/steady_timer.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/awaitable.hpp>

#include <etcd/etcd.h>

// 包含etcd客户端库 - 使用同步客户端
#include <etcd/SyncClient.hpp>
#include <etcd/Watcher.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Response.hpp>

namespace million {
namespace etcd {

// 服务注册信息
struct ServiceInfo {
    std::string address;
    int64_t lease_id;
    std::shared_ptr<::etcd::KeepAlive> keep_alive;
    std::shared_ptr<asio::steady_timer> heartbeat_timer;  // 仅用于监控keep_alive状态
};

// 监听器信息 - 使用真正的事件驱动
struct WatchInfo {
    std::string key;
    ServiceHandle subscriber;
    std::unique_ptr<::etcd::Watcher> watcher;  // etcd事件驱动watcher
};

// 定时消息类型
MILLION_MESSAGE_DEFINE_EMPTY(MILLION_ETCD_API, EtcdHeartbeatMsg);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdConfigChangedMsg, (std::string) key, (std::string) new_value, (std::string) old_value);

// 内部辅助消息类型
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, WatcherCreatedMsg, (std::string) key, (std::unique_ptr<::etcd::Watcher>) watcher);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ConfigChangeEventMsg, (std::string) key, (std::string) new_value, (std::string) old_value);

class EtcdService : public IService {
    MILLION_SERVICE_DEFINE(EtcdService);

public:
    using Base = IService;
    EtcdService(IMillion* imillion)
        : Base(imillion) {}

    virtual ~EtcdService();

    virtual bool OnInit() override;
    virtual Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override;
    virtual Task<MessagePointer> OnStop(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override;

    // 基本键值操作 - 同步版本
    MILLION_MESSAGE_HANDLE(EtcdKeyExistReq, msg) {
        bool exist = false;
        if (client_) {
            try {
                auto resp = client_->get(msg->key);
                exist = resp.is_ok() && !resp.value().as_string().empty();
            } catch (const std::exception& e) {
                logger().LOG_ERROR("EtcdKeyExistReq failed: {}", e.what());
                exist = false;
            }
        }
        co_return make_message<EtcdKeyExistResp>(exist);
    }

    MILLION_MESSAGE_HANDLE(EtcdSetReq, msg) {
        bool success = false;
        if (client_) {
            try {
                auto resp = client_->set(msg->key, msg->value);
                success = resp.is_ok();
            } catch (const std::exception& e) {
                logger().LOG_ERROR("EtcdSetReq failed: {}", e.what());
                success = false;
            }
        }
        co_return make_message<EtcdSetResp>(success);
    }

    MILLION_MESSAGE_HANDLE(EtcdGetReq, msg) {
        std::optional<std::string> value;
        if (client_) {
            try {
                auto resp = client_->get(msg->key);
                if (resp.is_ok()) {
                    value = resp.value().as_string();
                } else if (resp.error_code() != 100) { // 100 = key not found
                    logger().LOG_ERROR("EtcdGetReq failed: {}", resp.error_message());
                }
            } catch (const std::exception& e) {
                logger().LOG_ERROR("EtcdGetReq failed: {}", e.what());
            }
        }
        co_return make_message<EtcdGetResp>(value);
    }

    MILLION_MESSAGE_HANDLE(EtcdDelReq, msg) {
        bool success = false;
        if (client_) {
            try {
                auto resp = client_->rm(msg->key);
                success = resp.is_ok();
            } catch (const std::exception& e) {
                logger().LOG_ERROR("EtcdDelReq failed: {}", e.what());
                success = false;
            }
        }
        co_return make_message<EtcdDelResp>(success);
    }

    MILLION_MESSAGE_HANDLE(EtcdGetAllReq, msg) {
        std::map<std::string, std::string> kvs;
        if (client_) {
            try {
                auto resp = client_->ls(msg->prefix);
                if (resp.is_ok()) {
                    for (int i = 0; i < resp.keys_size(); ++i) {
                        kvs[resp.key(i)] = resp.value(i).as_string();
                    }
                }
            } catch (const std::exception& e) {
                logger().LOG_ERROR("EtcdGetAllReq failed: {}", e.what());
            }
        }
        co_return make_message<EtcdGetAllResp>(kvs);
    }

    // 服务注册与发现 - 同步版本
    MILLION_MESSAGE_HANDLE(EtcdServiceRegisterReq, msg) {
        bool success = co_await RegisterServiceWithLeaseAsync(msg->service_name, msg->service_address, msg->ttl);
        co_return make_message<EtcdServiceRegisterResp>(success);
    }

    MILLION_MESSAGE_HANDLE(EtcdServiceUnregisterReq, msg) {
        bool success = co_await UnregisterServiceAsync(msg->service_name, msg->service_address);
        co_return make_message<EtcdServiceUnregisterResp>(success);
    }

    MILLION_MESSAGE_HANDLE(EtcdServiceDiscoverReq, msg) {
        auto addresses = co_await DiscoverServicesAsync(msg->service_name);
        co_return make_message<EtcdServiceDiscoverResp>(addresses);
    }

    // 配置监听
    MILLION_MESSAGE_HANDLE(EtcdWatchReq, msg) {
        StartConfigWatch(msg->key, service_handle());
        co_return make_message<EtcdWatchResp>();
    }

    MILLION_MESSAGE_HANDLE(EtcdConfigWatchReq, msg) {
        StartConfigWatch(msg->config_key, service_handle());
        co_return make_message<EtcdConfigWatchResp>();
    }

    MILLION_MESSAGE_HANDLE(EtcdConfigLoadReq, msg) {
        std::optional<std::string> config_value;
        if (client_) {
            try {
                auto resp = client_->get(msg->config_key);
                if (resp.is_ok()) {
                    config_value = resp.value().as_string();
                } else if (resp.error_code() != 100) {
                    logger().LOG_ERROR("EtcdConfigLoadReq failed: {}", resp.error_message());
                }
            } catch (const std::exception& e) {
                logger().LOG_ERROR("EtcdConfigLoadReq failed: {}", e.what());
            }
        }
        co_return make_message<EtcdConfigLoadResp>(config_value);
    }

private:
    // 服务注册相关 - 异步版本（内部仍然使用同步客户端）
    Task<bool> RegisterServiceWithLeaseAsync(const std::string& service_name, const std::string& service_address, int ttl);
    Task<bool> UnregisterServiceAsync(const std::string& service_name, const std::string& service_address);
    Task<std::vector<std::string>> DiscoverServicesAsync(const std::string& service_name);

    // 配置监听相关 - 使用真正的事件驱动
    void StartConfigWatch(const std::string& config_key, const ServiceHandle& subscriber);
    void StopConfigWatch(const std::string& config_key);
    
    // etcd事件回调 - 在etcd的线程中调用，需要安全地发送到Service线程
    void OnEtcdWatchEvent(const std::string& key, const ::etcd::Response& response);
    
    // 心跳监控协程 - 仅用于监控keep_alive状态
    asio::awaitable<void> HeartbeatMonitorCoroutine(const std::string& service_key);

private:
    std::string endpoints_;
    std::unique_ptr<::etcd::SyncClient> client_;  // 使用同步客户端
    
    // 使用普通容器，因为Service是单线程的
    std::unordered_map<std::string, ServiceInfo> registered_services_;  // key: service_name:address
    std::unordered_map<std::string, WatchInfo> config_watchers_;         // key: config_key
    
    // 配置参数
    uint32_t heartbeat_check_interval_ms_ = 30000; // 心跳检查间隔
};

} // namespace etcd
} // namespace million