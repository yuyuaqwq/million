#include "etcd_service.h"
#include <yaml-cpp/yaml.h>

namespace million {
namespace etcd {

bool EtcdService::OnInit() {
    logger().LOG_INFO("EtcdService Init");

    // 设置服务名并启用独立工作线程
    imillion().SetServiceName(service_handle(), kEtcdServiceName);
    imillion().EnableSeparateWorker(service_handle());

    // 读取配置
    const auto& settings = imillion().YamlSettings();
    const auto& etcd_settings = settings["etcd"];
    if (!etcd_settings) {
        logger().LOG_ERROR("cannot find 'etcd' configuration.");
        return false;
    }

    const auto& endpoints = etcd_settings["endpoints"];
    if (!endpoints) {
        logger().LOG_ERROR("cannot find 'etcd.endpoints' configuration.");
        return false;
    }

    etcd_endpoints_ = endpoints.as<std::string>();
    logger().LOG_INFO("Using etcd endpoints: {}", etcd_endpoints_);

    // 初始化etcd客户端
    if (!InitializeClients()) {
        logger().LOG_ERROR("Failed to initialize etcd clients");
        return false;
    }

    return true;
}

bool EtcdService::InitializeClients() {
    try {
        // 初始化同步客户端
        sync_client_ = std::make_unique<::etcd::SyncClient>(etcd_endpoints_);
        
        // 测试连接
        auto response = sync_client_->head();
        if (!response.is_ok()) {
            logger().LOG_ERROR("Failed to connect to etcd: {}", response.error_message());
            return false;
        }
        
        logger().LOG_INFO("Successfully connected to etcd");
        return true;
        
    } catch (const std::exception& ex) {
        logger().LOG_ERROR("Exception while initializing etcd clients: {}", ex.what());
        return false;
    }
}

std::string EtcdService::ConvertEtcdError(const ::etcd::Response& response) {
    if (response.is_ok()) {
        return "";
    }
    return response.error_message();
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdGetReq, msg) {
    try {
        auto response = sync_client_->get(msg->key);
        
        auto reply = std::make_unique<EtcdGetResp>();
        reply->success = response.is_ok();
        reply->error_message = ConvertEtcdError(response);
        
        if (response.is_ok() && response.value().as_string() != "") {
            reply->value = response.value().as_string();
        }
        
        logger().LOG_DEBUG("EtcdGet key={}, success={}", msg->key, reply->success);
        Reply(sender, session_id, std::move(reply));
        
    } catch (const std::exception& ex) {
        auto reply = std::make_unique<EtcdGetResp>();
        reply->success = false;
        reply->error_message = ex.what();
        Reply(sender, session_id, std::move(reply));
        logger().LOG_ERROR("Exception in EtcdGet: {}", ex.what());
    }
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdPutReq, msg) {
    try {
        ::etcd::Response response;
        if (msg->lease_id > 0) {
            response = sync_client_->put(msg->key, msg->value, msg->lease_id);
        } else {
            response = sync_client_->put(msg->key, msg->value);
        }
        
        auto reply = std::make_unique<EtcdPutResp>();
        reply->success = response.is_ok();
        reply->error_message = ConvertEtcdError(response);
        
        logger().LOG_DEBUG("EtcdPut key={}, value={}, success={}", 
                          msg->key, msg->value, reply->success);
        Reply(sender, session_id, std::move(reply));
        
    } catch (const std::exception& ex) {
        auto reply = std::make_unique<EtcdPutResp>();
        reply->success = false;
        reply->error_message = ex.what();
        Reply(sender, session_id, std::move(reply));
        logger().LOG_ERROR("Exception in EtcdPut: {}", ex.what());
    }
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdDeleteReq, msg) {
    try {
        auto response = sync_client_->rm(msg->key);
        
        auto reply = std::make_unique<EtcdDeleteResp>();
        reply->success = response.is_ok();
        reply->error_message = ConvertEtcdError(response);
        
        logger().LOG_DEBUG("EtcdDelete key={}, success={}", msg->key, reply->success);
        Reply(sender, session_id, std::move(reply));
        
    } catch (const std::exception& ex) {
        auto reply = std::make_unique<EtcdDeleteResp>();
        reply->success = false;
        reply->error_message = ex.what();
        Reply(sender, session_id, std::move(reply));
        logger().LOG_ERROR("Exception in EtcdDelete: {}", ex.what());
    }
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdWatchReq, msg) {
    try {
        uint64_t watch_id = next_watch_id_++;
        watch_callbacks_[watch_id] = msg->callback_service;
        
        // 创建watch回调函数
        auto watch_callback = [this, watch_id, key = msg->key](::etcd::Response response) {
            try {
                if (response.error_code()) {
                    logger().LOG_ERROR("Watch error for key {}: {}", key, response.error_message());
                    return;
                }
                
                // 查找对应的回调服务
                auto it = watch_callbacks_.find(watch_id);
                if (it != watch_callbacks_.end()) {
                    auto event = std::make_unique<EtcdWatchEvent>();
                    event->key = key;
                    event->watch_id = watch_id;
                    
                    // 根据响应类型设置事件信息
                    if (response.action() == "set" || response.action() == "update") {
                        event->event_type = "PUT";
                        event->value = response.value().as_string();
                    } else if (response.action() == "delete") {
                        event->event_type = "DELETE";
                        event->value = response.prev_value().as_string();
                    } else {
                        event->event_type = response.action();
                        event->value = response.value().as_string();
                    }
                    
                    // 发送事件到回调服务
                    Send(it->second, std::move(event));
                    logger().LOG_DEBUG("Sent watch event for key={}, type={}, watch_id={}", 
                                      key, event->event_type, watch_id);
                }
            } catch (const std::exception& ex) {
                logger().LOG_ERROR("Exception in watch callback: {}", ex.what());
            }
        };
        
        // 创建Watcher
        auto watcher = std::make_unique<::etcd::Watcher>(*sync_client_, msg->key, watch_callback, false);
        watchers_[watch_id] = std::move(watcher);
        
        auto reply = std::make_unique<EtcdWatchResp>();
        reply->success = true;
        reply->watch_id = watch_id;
        reply->error_message = "";
        
        logger().LOG_DEBUG("EtcdWatch key={}, watch_id={}", msg->key, watch_id);
        Reply(sender, session_id, std::move(reply));
        
    } catch (const std::exception& ex) {
        auto reply = std::make_unique<EtcdWatchResp>();
        reply->success = false;
        reply->watch_id = 0;
        reply->error_message = ex.what();
        Reply(sender, session_id, std::move(reply));
        logger().LOG_ERROR("Exception in EtcdWatch: {}", ex.what());
    }
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdUnwatchReq, msg) {
    auto callback_it = watch_callbacks_.find(msg->watch_id);
    auto watcher_it = watchers_.find(msg->watch_id);
    
    bool found = (callback_it != watch_callbacks_.end() && watcher_it != watchers_.end());
    
    if (found) {
        // 取消Watcher
        watcher_it->second->Cancel();
        
        // 清理
        watch_callbacks_.erase(callback_it);
        watchers_.erase(watcher_it);
    }
    
    auto reply = std::make_unique<EtcdUnwatchResp>();
    reply->success = found;
    reply->error_message = found ? "" : "Watch ID not found";
    
    logger().LOG_DEBUG("EtcdUnwatch watch_id={}, success={}", msg->watch_id, reply->success);
    Reply(sender, session_id, std::move(reply));
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdBatchGetReq, msg) {
    auto reply = std::make_unique<EtcdBatchGetResp>();
    reply->success = true;
    reply->error_message = "";
    
    try {
        for (const auto& key : msg->keys) {
            auto response = sync_client_->get(key);
            if (response.is_ok() && response.value().as_string() != "") {
                reply->key_values.emplace_back(key, response.value().as_string());
            }
        }
        
        logger().LOG_DEBUG("EtcdBatchGet {} keys, found {} values", 
                          msg->keys.size(), reply->key_values.size());
        Reply(sender, session_id, std::move(reply));
        
    } catch (const std::exception& ex) {
        reply->success = false;
        reply->error_message = ex.what();
        Reply(sender, session_id, std::move(reply));
        logger().LOG_ERROR("Exception in EtcdBatchGet: {}", ex.what());
    }
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdLeaseGrantReq, msg) {
    try {
        auto response = sync_client_->leasegrant(msg->ttl);
        
        auto reply = std::make_unique<EtcdLeaseGrantResp>();
        reply->success = response.is_ok();
        reply->error_message = ConvertEtcdError(response);
        
        if (response.is_ok()) {
            reply->lease_id = response.value().lease();
        } else {
            reply->lease_id = 0;
        }
        
        logger().LOG_DEBUG("EtcdLeaseGrant ttl={}, lease_id={}, success={}", 
                          msg->ttl, reply->lease_id, reply->success);
        Reply(sender, session_id, std::move(reply));
        
    } catch (const std::exception& ex) {
        auto reply = std::make_unique<EtcdLeaseGrantResp>();
        reply->success = false;
        reply->lease_id = 0;
        reply->error_message = ex.what();
        Reply(sender, session_id, std::move(reply));
        logger().LOG_ERROR("Exception in EtcdLeaseGrant: {}", ex.what());
    }
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdLeaseRevokeReq, msg) {
    try {
        auto response = sync_client_->leaserevoke(msg->lease_id);
        
        auto reply = std::make_unique<EtcdLeaseRevokeResp>();
        reply->success = response.is_ok();
        reply->error_message = ConvertEtcdError(response);
        
        logger().LOG_DEBUG("EtcdLeaseRevoke lease_id={}, success={}", 
                          msg->lease_id, reply->success);
        Reply(sender, session_id, std::move(reply));
        
    } catch (const std::exception& ex) {
        auto reply = std::make_unique<EtcdLeaseRevokeResp>();
        reply->success = false;
        reply->error_message = ex.what();
        Reply(sender, session_id, std::move(reply));
        logger().LOG_ERROR("Exception in EtcdLeaseRevoke: {}", ex.what());
    }
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdListKeysReq, msg) {
    try {
        // etcd-cpp-apiv3使用ls方法列出键
        auto response = sync_client_->ls(msg->prefix);
        
        auto reply = std::make_unique<EtcdListKeysResp>();
        reply->success = response.is_ok();
        reply->error_message = ConvertEtcdError(response);
        
        if (response.is_ok()) {
            for (const auto& key : response.keys()) {
                reply->keys.push_back(key);
            }
        }
        
        logger().LOG_DEBUG("EtcdListKeys prefix={}, found {} keys, success={}", 
                          msg->prefix, reply->keys.size(), reply->success);
        Reply(sender, session_id, std::move(reply));
        
    } catch (const std::exception& ex) {
        auto reply = std::make_unique<EtcdListKeysResp>();
        reply->success = false;
        reply->error_message = ex.what();
        Reply(sender, session_id, std::move(reply));
        logger().LOG_ERROR("Exception in EtcdListKeys: {}", ex.what());
    }
}

} // namespace etcd
} // namespace million 