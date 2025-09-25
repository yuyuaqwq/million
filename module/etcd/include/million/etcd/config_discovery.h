#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>

#include <million/imillion.h>
#include <million/etcd/api.h>

namespace million {
namespace etcd {

// 配置发现请求和响应
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ConfigGetReq, (std::string) config_path);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ConfigGetResp, (bool) success, (std::optional<std::string>) config_value, (std::string) error_message);

MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ConfigSetReq, (std::string) config_path, (std::string) config_value);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ConfigSetResp, (bool) success, (std::string) error_message);

// 配置监听请求和响应
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ConfigWatchReq, (std::string) config_path, (ServiceHandle) callback_service);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ConfigWatchResp, (bool) success, (uint64_t) watch_id, (std::string) error_message);

// 配置变更事件
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ConfigChangeEvent, (std::string) config_path, (std::string) old_value, (std::string) new_value, (std::string) change_type, (uint64_t) watch_id);

// 服务注册和发现
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ServiceRegisterReq, (std::string) service_name, (std::string) service_address, (int32_t) service_port, (std::vector<std::string>) tags, (int64_t) ttl);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ServiceRegisterResp, (bool) success, (int64_t) lease_id, (std::string) error_message);

MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ServiceDiscoverReq, (std::string) service_name);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ServiceDiscoverResp, (bool) success, (std::vector<std::string>) service_endpoints, (std::string) error_message);

MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ServiceUnregisterReq, (int64_t) lease_id);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ServiceUnregisterResp, (bool) success, (std::string) error_message);

// 服务健康检查
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ServiceHeartbeatReq, (int64_t) lease_id);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, ServiceHeartbeatResp, (bool) success, (std::string) error_message);

} // namespace etcd
} // namespace million 