#include "etcd_service.h"

#include <yaml-cpp/yaml.h>
#include <chrono>

namespace million {
namespace etcd {

EtcdService::~EtcdService() {
    // etcd Watcher会自动清理
}

bool EtcdService::OnInit() {
    logger().LOG_INFO("EtcdService Init");

    // Set service name
    imillion().SetServiceName(service_handle(), kEtcdServiceName);

    // Read configuration
    const auto& settings = imillion().YamlSettings();
    const auto& etcd_settings = settings["etcd"];
    if (!etcd_settings) {
        logger().LOG_WARN("cannot find 'etcd' configuration.");
        endpoints_ = "http://127.0.0.1:2379"; // 默认值
    } else {
        // Get etcd configuration values
        const auto& etcd_endpoints = etcd_settings["endpoints"];
        if (etcd_endpoints) {
            endpoints_ = etcd_endpoints.as<std::string>();
        } else {
            endpoints_ = "http://127.0.0.1:2379"; // 默认值
        }
        
        // 读取心跳检查间隔配置
        const auto& heartbeat_interval = etcd_settings["heartbeat_check_interval_ms"];
        if (heartbeat_interval) {
            heartbeat_check_interval_ms_ = heartbeat_interval.as<uint32_t>();
        }
    }
    
    logger().LOG_INFO("Etcd endpoints: {}, heartbeat_check_interval: {}ms", 
                     endpoints_, heartbeat_check_interval_ms_);
    return true;
}

Task<MessagePointer> EtcdService::OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) {
    try {
        if (!endpoints_.empty()) {
            // 初始化同步etcd客户端
            client_ = std::make_unique<::etcd::SyncClient>(endpoints_);
            
            // 测试连接
            try {
                auto resp = client_->get("test_connection");
                bool success = resp.is_ok() || resp.error_code() == 100; // 100表示key不存在，这是正常的
                
                if (success) {
                    logger().LOG_INFO("Etcd sync client connected and tested successfully on {}", endpoints_);
                } else {
                    logger().LOG_WARN("Etcd connection test returned error: {}", resp.error_message());
                }
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Etcd connection test failed: {}", e.what());
            }
        }
    }
    catch (const std::exception& e) {
        logger().LOG_ERROR("Etcd service start failed: {}", e.what());
    }
    co_return nullptr;
}

Task<MessagePointer> EtcdService::OnStop(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) {
    // 停止所有watch
    for (auto& [key, watch_info] : config_watchers_) {
        try {
            if (watch_info.watcher) {
                watch_info.watcher->Cancel();  // 取消etcd watcher
            }
        } catch (const std::exception& e) {
            logger().LOG_ERROR("Exception while stopping watcher for key '{}': {}", key, e.what());
        }
    }
    config_watchers_.clear();
    
    // 停止所有心跳监控
    for (auto& [service_key, info] : registered_services_) {
        try {
            // 停止心跳监控定时器
            if (info.heartbeat_timer) {
                info.heartbeat_timer->cancel();
            }
            
            // 停止心跳保持
            if (info.keep_alive) {
                info.keep_alive->Cancel();
            }
            
            // 删除服务注册键 - 同步执行
            if (client_) {
                size_t colon_pos = service_key.find(':');
                if (colon_pos != std::string::npos) {
                    std::string service_name = service_key.substr(0, colon_pos);
                    std::string address = service_key.substr(colon_pos + 1);
                    std::string etcd_key = "/services/" + service_name + "/" + address;
                    
                    try {
                        auto resp = client_->rm(etcd_key);
                        if (resp.is_ok()) {
                            logger().LOG_INFO("Service {} unregistered successfully", service_name);
                        } else {
                            logger().LOG_WARN("Failed to unregister service {}: {}", service_name, resp.error_message());
                        }
                    } catch (const std::exception& e) {
                        logger().LOG_ERROR("Exception while unregistering service {}: {}", service_name, e.what());
                    }
                }
            }
        } catch (const std::exception& e) {
            logger().LOG_ERROR("Exception while cleaning up service {}: {}", service_key, e.what());
        }
    }
    registered_services_.clear();
    
    // 清理 etcd 客户端
    client_.reset();
    
    co_return nullptr;
}

Task<bool> EtcdService::RegisterServiceWithLeaseAsync(const std::string& service_name, const std::string& service_address, int ttl) {
    if (!client_) {
        logger().LOG_ERROR("Etcd client not initialized");
        co_return false;
    }
    
    try {
        // 创建租约 - 同步执行
        auto lease_resp = client_->leasegrant(ttl);
        if (!lease_resp.is_ok()) {
            logger().LOG_ERROR("Failed to create lease for service {}: {}", service_name, lease_resp.error_message());
            co_return false;
        }
        
        int64_t lease_id = lease_resp.value().lease();
        logger().LOG_INFO("Created lease {} with TTL {} for service {}", lease_id, ttl, service_name);
        
        // 注册服务 - 同步执行
        std::string service_key = "/services/" + service_name + "/" + service_address;
        std::string service_info = service_address; // 可以扩展为JSON格式包含更多信息
        
        auto set_resp = client_->set(service_key, service_info, lease_id);
        if (!set_resp.is_ok()) {
            logger().LOG_ERROR("Failed to register service {} at {}: {}", service_name, service_address, set_resp.error_message());
            co_return false;
        }
        
        // 开始心跳保持 - 同步创建
        try {
            auto keep_alive = client_->leasekeepalive(ttl);
            
            // 保存服务信息
            ServiceInfo info;
            info.address = service_address;
            info.lease_id = lease_id;
            info.keep_alive = keep_alive;
            
            // 创建心跳监控定时器（可选，用于监控keep_alive状态）
            std::string service_key_full = service_name + ":" + service_address;
            info.heartbeat_timer = std::make_shared<asio::steady_timer>(imillion().NextIoContext());
            
            registered_services_[service_key_full] = std::move(info);
            
            // 启动心跳监控协程（可选）
            auto& io_context = imillion().NextIoContext();
            asio::co_spawn(io_context, HeartbeatMonitorCoroutine(service_key_full), asio::detached);
            
            logger().LOG_INFO("Service {} registered at {} with lease {}", service_name, service_address, lease_id);
            co_return true;
            
        } catch (const std::exception& e) {
            logger().LOG_ERROR("Failed to create lease keep alive for service {}: {}", service_name, e.what());
            co_return false;
        }
        
    } catch (const std::exception& e) {
        logger().LOG_ERROR("Exception while registering service {}: {}", service_name, e.what());
        co_return false;
    }
}

Task<bool> EtcdService::UnregisterServiceAsync(const std::string& service_name, const std::string& service_address) {
    if (!client_) {
        logger().LOG_ERROR("Etcd client not initialized");
        co_return false;
    }
    
    try {
        std::string service_key_full = service_name + ":" + service_address;
        
        // 查找并清理服务信息
        auto it = registered_services_.find(service_key_full);
        if (it != registered_services_.end()) {
            // 停止心跳监控定时器
            if (it->second.heartbeat_timer) {
                it->second.heartbeat_timer->cancel();
            }
            
            // 停止心跳保持
            if (it->second.keep_alive) {
                it->second.keep_alive->Cancel();
            }
            registered_services_.erase(it);
        }
        
        // 删除服务注册键 - 同步执行
        std::string service_key = "/services/" + service_name + "/" + service_address;
        auto resp = client_->rm(service_key);
        
        if (resp.is_ok()) {
            logger().LOG_INFO("Service {} at {} unregistered successfully", service_name, service_address);
            co_return true;
        } else {
            logger().LOG_ERROR("Failed to unregister service {} at {}: {}", service_name, service_address, resp.error_message());
            co_return false;
        }
        
    } catch (const std::exception& e) {
        logger().LOG_ERROR("Exception while unregistering service {}: {}", service_name, e.what());
        co_return false;
    }
}

Task<std::vector<std::string>> EtcdService::DiscoverServicesAsync(const std::string& service_name) {
    std::vector<std::string> service_addresses;
    
    if (!client_) {
        logger().LOG_ERROR("Etcd client not initialized");
        co_return service_addresses;
    }
    
    try {
        std::string service_prefix = "/services/" + service_name + "/";
        auto resp = client_->ls(service_prefix);
        
        if (resp.is_ok()) {
            for (int i = 0; i < resp.keys_size(); ++i) {
                std::string key = resp.key(i);
                std::string value = resp.value(i).as_string();
                
                // 提取服务地址（从key的最后部分或从value中）
                if (!value.empty()) {
                    service_addresses.push_back(value);
                } else {
                    // 如果value为空，从key中提取地址
                    size_t last_slash = key.find_last_of('/');
                    if (last_slash != std::string::npos) {
                        service_addresses.push_back(key.substr(last_slash + 1));
                    }
                }
            }
        }
        
        logger().LOG_INFO("Discovered {} instances of service '{}'", service_addresses.size(), service_name);
        
    } catch (const std::exception& e) {
        logger().LOG_ERROR("Exception while discovering service '{}': {}", service_name, e.what());
    }
    
    co_return service_addresses;
}

void EtcdService::StartConfigWatch(const std::string& config_key, const ServiceHandle& subscriber) {
    if (!client_) {
        logger().LOG_ERROR("Etcd client not initialized");
        return;
    }
    
    try {
        // 如果已经在监听这个键，更新订阅者并重新创建watcher
        auto it = config_watchers_.find(config_key);
        if (it != config_watchers_.end()) {
            // 取消旧的watcher
            if (it->second.watcher) {
                it->second.watcher->Cancel();
            }
            // 更新订阅者
            it->second.subscriber = subscriber;
        } else {
            // 创建新的监听器信息
            WatchInfo watch_info;
            watch_info.key = config_key;
            watch_info.subscriber = subscriber;
            config_watchers_[config_key] = std::move(watch_info);
            it = config_watchers_.find(config_key);
        }
        
        // 创建etcd事件驱动watcher - 注意这仍然需要异步回调机制
        try {
            // 注意：这个回调会在etcd客户端的线程中被调用
            auto callback = [this, config_key](const ::etcd::Response& response) {
                this->OnEtcdWatchEvent(config_key, response);
            };
            
            // 创建watcher，监听指定key的变化
            // 注意：即使使用SyncClient，Watcher仍然是异步的
            auto watcher = std::make_unique<::etcd::Watcher>(
                *client_, 
                config_key, 
                callback,
                false  // 不递归监听
            );
            
            // 直接保存watcher
            it->second.watcher = std::move(watcher);
            
            logger().LOG_INFO("Started event-driven watching for config key '{}'", config_key);
        } catch (const std::exception& e) {
            logger().LOG_ERROR("Exception while creating watcher for key '{}': {}", config_key, e.what());
        }
        
    } catch (const std::exception& e) {
        logger().LOG_ERROR("Exception while starting config watch for key '{}': {}", config_key, e.what());
    }
}

// 处理watcher创建完成的消息
MILLION_MESSAGE_HANDLE(WatcherCreatedMsg, msg) {
    auto it = config_watchers_.find(msg->key);
    if (it != config_watchers_.end()) {
        it->second.watcher = std::move(msg->watcher);
        logger().LOG_INFO("Watcher for key '{}' created successfully", msg->key);
    }
    co_return nullptr;
}

void EtcdService::StopConfigWatch(const std::string& config_key) {
    auto it = config_watchers_.find(config_key);
    if (it != config_watchers_.end()) {
        if (it->second.watcher) {
            it->second.watcher->Cancel();
        }
        config_watchers_.erase(it);
        logger().LOG_INFO("Stopped watching config key '{}'", config_key);
    }
}

void EtcdService::OnEtcdWatchEvent(const std::string& key, const ::etcd::Response& response) {
    try {
        // 这个函数在etcd客户端的线程中被调用
        // 我们需要安全地将事件发送到Service线程
        
        if (!response.is_ok()) {
            logger().LOG_ERROR("Watch error for key '{}': {}", key, response.error_message());
            return;
        }
        
        // 处理事件
        for (const auto& event : response.events()) {
            std::string new_value;
            std::string old_value;
            
            if (event.event_type() == ::etcd::Event::EventType::PUT) {
                new_value = event.kv().as_string();
                old_value = event.prev_kv().as_string();
                
                logger().LOG_INFO("Config key '{}' changed: '{}' -> '{}'", key, old_value, new_value);
            } else if (event.event_type() == ::etcd::Event::EventType::DELETE_) {
                new_value = "";  // key被删除
                old_value = event.prev_kv().as_string();
                
                logger().LOG_INFO("Config key '{}' deleted, previous value: '{}'", key, old_value);
            } else {
                continue;  // 忽略其他事件类型
            }
            
            // 发送事件通知消息到Service线程
            auto event_msg = make_message<ConfigChangeEventMsg>(key, new_value, old_value);
            imillion().SendTo(service_handle(), service_handle(), imillion().NewSession(), std::move(event_msg));
        }
        
    } catch (const std::exception& e) {
        logger().LOG_ERROR("Exception in etcd watch event handler for key '{}': {}", key, e.what());
    }
}

// 处理配置变更事件的消息
MILLION_MESSAGE_HANDLE(ConfigChangeEventMsg, msg) {
    auto it = config_watchers_.find(msg->key);
    if (it != config_watchers_.end()) {
        auto subscriber = it->second.subscriber;
        auto subscriber_lock = subscriber.lock();
        if (subscriber_lock) {
            auto change_msg = make_message<EtcdConfigChangedMsg>(msg->key, msg->new_value, msg->old_value);
            imillion().Send(service_handle(), subscriber, std::move(change_msg));
        } else {
            logger().LOG_WARN("Subscriber for key '{}' no longer exists", msg->key);
            // 可以考虑在这里清理无效的watcher
        }
    }
    co_return nullptr;
}

asio::awaitable<void> EtcdService::HeartbeatMonitorCoroutine(const std::string& service_key) {
    // 这个协程主要用于监控keep_alive的状态，etcd的keep_alive会自动处理心跳
    while (true) {
        try {
            // 检查服务是否还存在
            auto it = registered_services_.find(service_key);
            if (it == registered_services_.end()) {
                break; // 服务已被移除
            }
            
            // 等待检查间隔
            it->second.heartbeat_timer->expires_after(std::chrono::milliseconds(heartbeat_check_interval_ms_));
            co_await it->second.heartbeat_timer->async_wait(asio::use_awaitable);
            
            // 检查keep_alive是否正常（这里主要是日志记录）
            if (it->second.keep_alive) {
                logger().LOG_DEBUG("Heartbeat monitor check for service: {}", service_key);
                // keep_alive对象会自动处理与etcd服务器的心跳通信
                // 我们这里只是做状态监控，实际的心跳由keep_alive内部处理
            }
            
        } catch (const asio::system_error& e) {
            if (e.code() == asio::error::operation_aborted) {
                // 定时器被取消，正常退出
                break;
            }
            logger().LOG_ERROR("HeartbeatMonitorCoroutine error for service '{}': {}", service_key, e.what());
        } catch (const std::exception& e) {
            logger().LOG_ERROR("HeartbeatMonitorCoroutine exception for service '{}': {}", service_key, e.what());
            break;
        }
    }
    
    logger().LOG_INFO("HeartbeatMonitorCoroutine for service '{}' terminated", service_key);
}

} // namespace etcd
} // namespace million