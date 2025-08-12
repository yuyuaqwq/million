#pragma once

#include <string>
#include <vector>
#include <optional>

#include <million/imillion.h>

#include <etcd/api.h>

namespace million {
namespace etcd {

constexpr const char* kEtcdServiceName = "EtcdService";
#define MILLION_ETCD_SERVICE_NAME "EtcdService"

// ETCD 基础操作请求和响应
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdGetReq, (std::string) key);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdGetResp, (bool) success, (std::optional<std::string>) value, (std::string) error_message);

MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdPutReq, (std::string) key, (std::string) value, (int64_t) lease_id);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdPutResp, (bool) success, (std::string) error_message);

MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdDeleteReq, (std::string) key);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdDeleteResp, (bool) success, (std::string) error_message);

// ETCD 监听操作
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdWatchReq, (std::string) key, (ServiceHandle) callback_service);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdWatchResp, (bool) success, (uint64_t) watch_id, (std::string) error_message);

// ETCD 监听事件回调
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdWatchEvent, (std::string) key, (std::string) value, (std::string) event_type, (uint64_t) watch_id);

// ETCD 取消监听
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdUnwatchReq, (uint64_t) watch_id);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdUnwatchResp, (bool) success, (std::string) error_message);

// ETCD 批量操作
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdBatchGetReq, (std::vector<std::string>) keys);
using EtcdBatchGetRespKeyValues = std::vector<std::pair<std::string, std::string>>;
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdBatchGetResp, (bool) success, (EtcdBatchGetRespKeyValues) key_values, (std::string) error_message);

// ETCD 租约操作
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdLeaseGrantReq, (int64_t) ttl);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdLeaseGrantResp, (bool) success, (int64_t) lease_id, (std::string) error_message);

MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdLeaseRevokeReq, (int64_t) lease_id);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdLeaseRevokeResp, (bool) success, (std::string) error_message);

// ETCD 目录遍历
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdListKeysReq, (std::string) prefix);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdListKeysResp, (bool) success, (std::vector<std::string>) keys, (std::string) error_message);

} // namespace etcd
} // namespace million 