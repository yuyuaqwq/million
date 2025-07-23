#pragma once

#include <memory>
#include <string>
#include <map>
#include <functional>
#include <unordered_map>
#include <thread>
#include <atomic>

#include <million/imillion.h>

#include <etcd/etcd.h>

// 包含etcd客户端库
#include <etcd/SyncClient.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Response.hpp>
#include <etcd/Watcher.hpp>

namespace million {
namespace etcd {

// 服务注册信息
struct ServiceInfo {
    std::string address;
    int64_t lease_id;
    std::unique_ptr<::etcd::KeepAlive> keep_alive;
};

// 监听器信息
struct WatchInfo {
    std::string key;
    std::function<void(const std::string&)> callback;
    std::unique_ptr<::etcd::Watcher> watcher;
    std::thread watch_thread;
    std::atomic<bool> should_stop{false};
};

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

    // 基本键值操作
    MILLION_MESSAGE_HANDLE(EtcdKeyExistReq, msg) {
        bool exist = false;
        if (client_) {
            try {
                auto resp = client_->get(msg->key);
                exist = resp.is_ok() && !resp.value().as_string().empty();
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Failed to check key existence '{}': {}", msg->key, e.what());
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
                if (!success) {
                    logger().LOG_ERROR("Failed to set key '{}': {}", msg->key, resp.error_message());
                }
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Exception while setting key '{}': {}", msg->key, e.what());
            }
        }
        co_return make_message<EtcdSetResp>(success);
    }

    MILLION_MESSAGE_HANDLE(EtcdGetReq, msg) {
        std::optional<std::string> value;
        if (client_) {
            try {
                auto resp = client_->get(msg->key);
                if (resp.is_ok() && !resp.value().as_string().empty()) {
                    value = resp.value().as_string();
                } else if (!resp.is_ok()) {
                    logger().LOG_WARN("Failed to get key '{}': {}", msg->key, resp.error_message());
                }
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Exception while getting key '{}': {}", msg->key, e.what());
            }
        }
        co_return make_message<EtcdGetResp>(std::move(value));
    }

    MILLION_MESSAGE_HANDLE(EtcdDelReq, msg) {
        bool success = false;
        if (client_) {
            try {
                auto resp = client_->rm(msg->key);
                success = resp.is_ok();
                if (!success) {
                    logger().LOG_ERROR("Failed to delete key '{}': {}", msg->key, resp.error_message());
                }
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Exception while deleting key '{}': {}", msg->key, e.what());
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
                } else {
                    logger().LOG_ERROR("Failed to get all keys with prefix '{}': {}", msg->prefix, resp.error_message());
                }
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Exception while getting all keys with prefix '{}': {}", msg->prefix, e.what());
            }
        }
        co_return make_message<EtcdGetAllResp>(std::move(kvs));
    }

    MILLION_MESSAGE_HANDLE(EtcdWatchReq, msg) {
        if (client_) {
            try {
                // 这里可以实现监听逻辑，但需要额外的回调机制
                logger().LOG_INFO("Watch requested for key: {}", msg->key);
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Exception while setting up watch for key '{}': {}", msg->key, e.what());
            }
        }
        co_return make_message<EtcdWatchResp>();
    }

    // 配置相关处理
    MILLION_MESSAGE_HANDLE(EtcdConfigLoadReq, msg) {
        std::optional<std::string> config_value;
        if (client_) {
            try {
                auto resp = client_->get(msg->config_key);
                if (resp.is_ok() && !resp.value().as_string().empty()) {
                    config_value = resp.value().as_string();
                    logger().LOG_INFO("Loaded config from key '{}'", msg->config_key);
                } else if (!resp.is_ok()) {
                    logger().LOG_WARN("Failed to load config from key '{}': {}", msg->config_key, resp.error_message());
                }
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Exception while loading config from key '{}': {}", msg->config_key, e.what());
            }
        }
        co_return make_message<EtcdConfigLoadResp>(std::move(config_value));
    }

    MILLION_MESSAGE_HANDLE(EtcdConfigWatchReq, msg) {
        if (client_) {
            try {
                StartConfigWatch(msg->config_key);
                logger().LOG_INFO("Started watching config key: {}", msg->config_key);
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Exception while setting up config watch for key '{}': {}", msg->config_key, e.what());
            }
        }
        co_return make_message<EtcdConfigWatchResp>();
    }

    // 服务注册与发现处理
    MILLION_MESSAGE_HANDLE(EtcdServiceRegisterReq, msg) {
        bool success = false;
        if (client_) {
            try {
                success = RegisterServiceWithLease(msg->service_name, msg->service_address, msg->ttl);
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Exception while registering service '{}': {}", msg->service_name, e.what());
            }
        }
        co_return make_message<EtcdServiceRegisterResp>(success);
    }

    MILLION_MESSAGE_HANDLE(EtcdServiceUnregisterReq, msg) {
        bool success = false;
        if (client_) {
            try {
                success = UnregisterService(msg->service_name, msg->service_address);
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Exception while unregistering service '{}': {}", msg->service_name, e.what());
            }
        }
        co_return make_message<EtcdServiceUnregisterResp>(success);
    }

    MILLION_MESSAGE_HANDLE(EtcdServiceDiscoverReq, msg) {
        std::vector<std::string> service_addresses;
        if (client_) {
            try {
                service_addresses = DiscoverServices(msg->service_name);
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Exception while discovering service '{}': {}", msg->service_name, e.what());
            }
        }
        co_return make_message<EtcdServiceDiscoverResp>(std::move(service_addresses));
    }

private:
    // 辅助方法
    bool RegisterServiceWithLease(const std::string& service_name, const std::string& service_address, int ttl);
    bool UnregisterService(const std::string& service_name, const std::string& service_address);
    std::vector<std::string> DiscoverServices(const std::string& service_name);
    void StartConfigWatch(const std::string& config_key);
    void StopAllWatchers();

    // etcd 客户端指针
    std::unique_ptr<::etcd::SyncClient> client_;
    
    // etcd 服务器地址
    std::string endpoints_;
    
    // 注册的服务信息
    std::unordered_map<std::string, ServiceInfo> registered_services_;
    
    // 配置监听器
    std::unordered_map<std::string, std::unique_ptr<WatchInfo>> config_watchers_;
    
    // 保护数据结构的锁
    std::mutex services_mutex_;
    std::mutex watchers_mutex_;
};

} // namespace etcd
} // namespace million