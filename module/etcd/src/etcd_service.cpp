#include "etcd_service.h"
#include <yaml-cpp/yaml.h>

#include <etcd/ss_etcd.pb.h>

namespace million {
namespace etcd {

bool EtcdService::OnInit() {
    logger().LOG_INFO("EtcdService Init");

    // 设置服务名并启用独立工作线程
    imillion().SetServiceNameId(service_handle(), module::module_id, ss::ServiceNameId_descriptor(), ss::SERVICE_NAME_ID_ETCD);
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

    if (endpoints.IsSequence()) {
        // 多个endpoint的情况
        std::vector<std::string> endpoint_list = endpoints.as<std::vector<std::string>>();
        for (size_t i = 0; i < endpoint_list.size(); ++i) {
            if (i > 0) {
                etcd_endpoints_ += ",";
            }
            etcd_endpoints_ += endpoint_list[i];
        }
    } else {
        // 单个endpoint的情况（向后兼容）
        etcd_endpoints_ = endpoints.as<std::string>();
    }
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
        auto reply = std::make_unique<EtcdGetResp>(response.is_ok(), std::nullopt, ConvertEtcdError(response));
        if (response.is_ok() && response.value().as_string() != "") {
            reply->value = response.value().as_string();
        }
        logger().LOG_DEBUG("EtcdGet key={}, success={}", msg->key, reply->success);
        co_return std::move(reply);
        
    } catch (const std::exception& ex) {
        logger().LOG_ERROR("Exception in EtcdGet: {}", ex.what());
        co_return make_message<EtcdGetResp>(false, std::nullopt, ex.what());
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
        
        logger().LOG_DEBUG("EtcdPut key={}, value={}, success={}", 
                          msg->key, msg->value, response.is_ok());
        co_return make_message<EtcdPutResp>(response.is_ok(), ConvertEtcdError(response));
        
    } catch (const std::exception& ex) {
        logger().LOG_ERROR("Exception in EtcdPut: {}", ex.what());
        co_return make_message<EtcdPutResp>(false, ex.what());
    }
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdDeleteReq, msg) {
    try {
        auto response = sync_client_->rm(msg->key);
        
        logger().LOG_DEBUG("EtcdDelete key={}, success={}", msg->key, response.is_ok());
        co_return make_message<EtcdDeleteResp>(response.is_ok(), ConvertEtcdError(response));
    } catch (const std::exception& ex) {
        logger().LOG_ERROR("Exception in EtcdDelete: {}", ex.what());
        co_return make_message<EtcdDeleteResp>(false, ex.what());
    }
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdWatchReq, msg) {
    try {
        uint64_t watch_id = next_watch_id_++;
        watch_callbacks_[watch_id] = msg->callback_service;
        
        // 创建watch回调函数
        // 这里的生命周期/线程安全需要分析下是否存在问题
        auto watch_callback = [this, watch_id, key = msg->key](::etcd::Response response) {
            try {
                if (response.error_code()) {
                    logger().LOG_ERROR("Watch error for key {}: {}", key, response.error_message());
                    return;
                }
                
                // 查找对应的回调服务
                auto it = watch_callbacks_.find(watch_id);
                if (it != watch_callbacks_.end()) {
                    auto event = std::make_unique<EtcdWatchEvent>(key, "", "", watch_id);
                    
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
                    logger().LOG_DEBUG("Sent watch event for key={}, type={}, watch_id={}",
                        key, event->event_type, watch_id);
                    Send(it->second, std::move(event));
                }
            } catch (const std::exception& ex) {
                logger().LOG_ERROR("Exception in watch callback: {}", ex.what());
            }
        };
        
        // 创建Watcher
        auto watcher = std::make_unique<::etcd::Watcher>(*sync_client_, msg->key, watch_callback, false);
        watchers_[watch_id] = std::move(watcher);
        
        logger().LOG_DEBUG("EtcdWatch key={}, watch_id={}", msg->key, watch_id);
        co_return make_message<EtcdWatchResp>(true, watch_id, "");
        
    } catch (const std::exception& ex) {
        logger().LOG_ERROR("Exception in EtcdWatch: {}", ex.what());
        co_return make_message<EtcdWatchResp>(false, 0, ex.what());
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
    
    logger().LOG_DEBUG("EtcdUnwatch watch_id={}, success={}", msg->watch_id, found);
    co_return make_message<EtcdUnwatchResp>(found, found ? "" : "Watch ID not found");
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdBatchGetReq, msg) {
    try {
        EtcdBatchGetRespKeyValues key_values;
        for (const auto& key : msg->keys) {
            auto response = sync_client_->get(key);
            if (response.is_ok() && response.value().as_string() != "") {
                key_values.emplace_back(key, response.value().as_string());
            }
        }
        
        logger().LOG_DEBUG("EtcdBatchGet {} keys, found {} values", 
                          msg->keys.size(), key_values.size());
        co_return make_message<EtcdBatchGetResp>(true, std::move(key_values), "");
        
    } catch (const std::exception& ex) {
        logger().LOG_ERROR("Exception in EtcdBatchGet: {}", ex.what());
        co_return make_message<EtcdBatchGetResp>(false, EtcdBatchGetRespKeyValues(), ex.what());
    }
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdLeaseGrantReq, msg) {
    try {
        auto response = sync_client_->leasegrant(msg->ttl);

        int64_t lease_id = 0;
        if (response.is_ok()) {
            lease_id = response.value().lease();
        }
        
        logger().LOG_DEBUG("EtcdLeaseGrant ttl={}, lease_id={}, success={}", 
                          msg->ttl, lease_id, response.is_ok());
        co_return make_message<EtcdLeaseGrantResp>(response.is_ok(), lease_id, ConvertEtcdError(response));
        
    } catch (const std::exception& ex) {
        logger().LOG_ERROR("Exception in EtcdLeaseGrant: {}", ex.what());
        co_return make_message<EtcdLeaseGrantResp>(false, 0, ex.what());
    }
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdLeaseRevokeReq, msg) {
    try {
        auto response = sync_client_->leaserevoke(msg->lease_id);
        
        logger().LOG_DEBUG("EtcdLeaseRevoke lease_id={}, success={}", 
                          msg->lease_id, response.is_ok());
        co_return make_message<EtcdLeaseRevokeResp>(response.is_ok(), ConvertEtcdError(response));
        
    } catch (const std::exception& ex) {
        logger().LOG_ERROR("Exception in EtcdLeaseRevoke: {}", ex.what());
        co_return make_message<EtcdLeaseRevokeResp>(false, ex.what());
    }
}

MILLION_MESSAGE_HANDLE_IMPL(EtcdService, EtcdListKeysReq, msg) {
    try {
        // etcd-cpp-apiv3使用ls方法列出键
        auto response = sync_client_->ls(msg->prefix);
        
        std::vector<std::string> keys;

        if (response.is_ok()) {
            for (const auto& key : response.keys()) {
                keys.push_back(key);
            }
        }
        
        logger().LOG_DEBUG("EtcdListKeys prefix={}, found {} keys, success={}", 
                          msg->prefix, keys.size(), response.is_ok());
        co_return make_message<EtcdListKeysResp>( response.is_ok(), std::move(keys), ConvertEtcdError(response));
        
    } catch (const std::exception& ex) {
        logger().LOG_ERROR("Exception in EtcdListKeys: {}", ex.what());
        co_return make_message<EtcdListKeysResp>(false, std::vector<std::string>(), ex.what());
    }
}

} // namespace etcd
} // namespace million 