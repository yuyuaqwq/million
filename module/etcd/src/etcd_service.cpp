#include "etcd_service.h"

#include <yaml-cpp/yaml.h>
#include <chrono>

namespace million {
namespace etcd {

EtcdService::~EtcdService() {
    StopAllWatchers();
}

bool EtcdService::OnInit() {
    logger().LOG_INFO("EtcdService Init");

    // Set service name
    imillion().SetServiceName(service_handle(), kEtcdServiceName);
    
    // Enable separate worker if needed
    // imillion().EnableSeparateWorker(service_handle());

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
    }
    
    logger().LOG_INFO("Etcd endpoints: {}", endpoints_);
    return true;
}

Task<MessagePointer> EtcdService::OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) {
    try {
        if (!endpoints_.empty()) {
            // 初始化 etcd 同步客户端
            client_ = std::make_unique<::etcd::SyncClient>(endpoints_);
            logger().LOG_INFO("Etcd sync client connected to {}", endpoints_);
            
            // 测试连接
            auto resp = client_->get("test_connection");
            if (resp.is_ok() || resp.error_code() == 100) { // 100表示key不存在，这是正常的
                logger().LOG_INFO("Etcd connection test successful");
            } else {
                logger().LOG_WARN("Etcd connection test returned error: {}", resp.error_message());
            }
        }
    }
    catch (const std::exception& e) {
        logger().LOG_ERROR("Etcd client initialization failed: {}", e.what());
    }
    co_return nullptr;
}

Task<MessagePointer> EtcdService::OnStop(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) {
    // 清理注册的服务
    {
        std::lock_guard<std::mutex> lock(services_mutex_);
        for (auto& [service_name, info] : registered_services_) {
            try {
                // 停止心跳保持
                if (info.keep_alive) {
                    info.keep_alive->Cancel();
                }
                
                // 删除服务注册键
                std::string service_key = "/services/" + service_name + "/" + info.address;
                if (client_) {
                    auto resp = client_->rm(service_key);
                    if (resp.is_ok()) {
                        logger().LOG_INFO("Service {} unregistered successfully", service_name);
                    } else {
                        logger().LOG_WARN("Failed to unregister service {}: {}", service_name, resp.error_message());
                    }
                }
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Exception while cleaning up service {}: {}", service_name, e.what());
            }
        }
        registered_services_.clear();
    }
    
    // 停止所有监听器
    StopAllWatchers();
    
    // 清理 etcd 客户端
    client_.reset();
    
    co_return nullptr;
}

bool EtcdService::RegisterServiceWithLease(const std::string& service_name, const std::string& service_address, int ttl) {
    if (!client_) {
        logger().LOG_ERROR("Etcd client not initialized");
        return false;
    }
    
    try {
        // 创建租约
        auto lease_resp = client_->leasegrant(ttl);
        if (!lease_resp.is_ok()) {
            logger().LOG_ERROR("Failed to create lease for service {}: {}", service_name, lease_resp.error_message());
            return false;
        }
        
        int64_t lease_id = lease_resp.value().lease();
        logger().LOG_INFO("Created lease {} with TTL {} for service {}", lease_id, ttl, service_name);
        
        // 注册服务
        std::string service_key = "/services/" + service_name + "/" + service_address;
        std::string service_info = service_address; // 可以扩展为JSON格式包含更多信息
        
        auto set_resp = client_->set(service_key, service_info, lease_id);
        if (!set_resp.is_ok()) {
            logger().LOG_ERROR("Failed to register service {} at {}: {}", service_name, service_address, set_resp.error_message());
            return false;
        }
        
        // 开始心跳保持
        auto keep_alive = client_->leasekeepalive(ttl);
        if (!keep_alive) {
            logger().LOG_ERROR("Failed to create lease keep alive for service {}", service_name);
            return false;
        }
        
        // 保存服务信息
        {
            std::lock_guard<std::mutex> lock(services_mutex_);
            ServiceInfo info;
            info.address = service_address;
            info.lease_id = lease_id;
            info.keep_alive = std::move(keep_alive);
            
            std::string service_key_full = service_name + ":" + service_address;
            registered_services_[service_key_full] = std::move(info);
        }
        
        logger().LOG_INFO("Service {} registered at {} with lease {}", service_name, service_address, lease_id);
        return true;
        
    } catch (const std::exception& e) {
        logger().LOG_ERROR("Exception while registering service {}: {}", service_name, e.what());
        return false;
    }
}

bool EtcdService::UnregisterService(const std::string& service_name, const std::string& service_address) {
    if (!client_) {
        logger().LOG_ERROR("Etcd client not initialized");
        return false;
    }
    
    try {
        std::string service_key_full = service_name + ":" + service_address;
        
        // 查找并清理服务信息
        {
            std::lock_guard<std::mutex> lock(services_mutex_);
            auto it = registered_services_.find(service_key_full);
            if (it != registered_services_.end()) {
                // 停止心跳保持
                if (it->second.keep_alive) {
                    it->second.keep_alive->Cancel();
                }
                registered_services_.erase(it);
            }
        }
        
        // 删除服务注册键
        std::string service_key = "/services/" + service_name + "/" + service_address;
        auto resp = client_->rm(service_key);
        if (resp.is_ok()) {
            logger().LOG_INFO("Service {} at {} unregistered successfully", service_name, service_address);
            return true;
        } else {
            logger().LOG_ERROR("Failed to unregister service {} at {}: {}", service_name, service_address, resp.error_message());
            return false;
        }
        
    } catch (const std::exception& e) {
        logger().LOG_ERROR("Exception while unregistering service {}: {}", service_name, e.what());
        return false;
    }
}

std::vector<std::string> EtcdService::DiscoverServices(const std::string& service_name) {
    std::vector<std::string> service_addresses;
    
    if (!client_) {
        logger().LOG_ERROR("Etcd client not initialized");
        return service_addresses;
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
            logger().LOG_INFO("Discovered {} instances of service '{}'", service_addresses.size(), service_name);
        } else {
            logger().LOG_WARN("Failed to discover service '{}': {}", service_name, resp.error_message());
        }
        
    } catch (const std::exception& e) {
        logger().LOG_ERROR("Exception while discovering service '{}': {}", service_name, e.what());
    }
    
    return service_addresses;
}

void EtcdService::StartConfigWatch(const std::string& config_key) {
    if (!client_) {
        logger().LOG_ERROR("Etcd client not initialized");
        return;
    }
    
    try {
        std::lock_guard<std::mutex> lock(watchers_mutex_);
        
        // 如果已经在监听这个键，先停止
        auto it = config_watchers_.find(config_key);
        if (it != config_watchers_.end()) {
            it->second->should_stop = true;
            if (it->second->watch_thread.joinable()) {
                it->second->watch_thread.join();
            }
            config_watchers_.erase(it);
        }
        
        // 创建新的监听器
        auto watch_info = std::make_unique<WatchInfo>();
        watch_info->key = config_key;
        watch_info->should_stop = false;
        
        // 创建监听器线程
        watch_info->watch_thread = std::thread([this, config_key]() {
            try {
                auto watcher = client_->watch(config_key);
                while (true) {
                    {
                        std::lock_guard<std::mutex> lock(watchers_mutex_);
                        auto it = config_watchers_.find(config_key);
                        if (it == config_watchers_.end() || it->second->should_stop) {
                            break;
                        }
                    }
                    
                    auto resp = watcher->waitOnce();
                    if (resp.is_ok()) {
                        for (const auto& event : resp.events()) {
                            if (event.event_type() == ::etcd::Event::EventType::PUT) {
                                logger().LOG_INFO("Config key '{}' changed to: {}", config_key, event.kv().value());
                                // 这里可以触发回调，但需要额外的机制来存储回调函数
                            }
                        }
                    } else {
                        logger().LOG_ERROR("Watch error for key '{}': {}", config_key, resp.error_message());
                        break;
                    }
                }
            } catch (const std::exception& e) {
                logger().LOG_ERROR("Exception in watch thread for key '{}': {}", config_key, e.what());
            }
        });
        
        config_watchers_[config_key] = std::move(watch_info);
        
    } catch (const std::exception& e) {
        logger().LOG_ERROR("Exception while starting config watch for key '{}': {}", config_key, e.what());
    }
}

void EtcdService::StopAllWatchers() {
    std::lock_guard<std::mutex> lock(watchers_mutex_);
    
    for (auto& [key, watch_info] : config_watchers_) {
        try {
            watch_info->should_stop = true;
            if (watch_info->watch_thread.joinable()) {
                watch_info->watch_thread.join();
            }
        } catch (const std::exception& e) {
            logger().LOG_ERROR("Exception while stopping watcher for key '{}': {}", key, e.what());
        }
    }
    
    config_watchers_.clear();
}

} // namespace etcd
} // namespace million