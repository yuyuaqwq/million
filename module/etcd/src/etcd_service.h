#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <etcd/SyncClient.hpp>
#include <etcd/Response.hpp>
#include <etcd/Watcher.hpp>

#include <million/imillion.h>

#include <etcd/etcd.h>

namespace million {
namespace etcd {

class EtcdService : public IService {
    MILLION_SERVICE_DEFINE(EtcdService);

public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override;

private:
    std::unique_ptr<::etcd::SyncClient> sync_client_;
    std::string etcd_endpoints_;
    
    // 监听管理
    uint64_t next_watch_id_ = 1;
    std::unordered_map<uint64_t, ServiceHandle> watch_callbacks_;
    std::unordered_map<uint64_t, std::unique_ptr<::etcd::Watcher>> watchers_;

public:
    // 基础操作消息处理
    MILLION_MESSAGE_HANDLE(EtcdGetReq, msg);
    MILLION_MESSAGE_HANDLE(EtcdPutReq, msg);
    MILLION_MESSAGE_HANDLE(EtcdDeleteReq, msg);
    
    // 监听操作消息处理
    MILLION_MESSAGE_HANDLE(EtcdWatchReq, msg);
    MILLION_MESSAGE_HANDLE(EtcdUnwatchReq, msg);
    
    // 批量操作消息处理
    MILLION_MESSAGE_HANDLE(EtcdBatchGetReq, msg);
    
    // 租约操作消息处理
    MILLION_MESSAGE_HANDLE(EtcdLeaseGrantReq, msg);
    MILLION_MESSAGE_HANDLE(EtcdLeaseRevokeReq, msg);
    
    // 目录遍历消息处理
    MILLION_MESSAGE_HANDLE(EtcdListKeysReq, msg);

private:
    bool InitializeClients();
    std::string ConvertEtcdError(const ::etcd::Response& response);
};

} // namespace etcd
} // namespace million 