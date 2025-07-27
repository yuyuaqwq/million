#pragma once

#include <string>
#include <optional>
#include <vector>
#include <map>

#include <million/imillion.h>

// 定义API导出宏
#ifdef _WIN32
    #ifdef BUILDING_ETCD_MODULE
        #define MILLION_ETCD_API __declspec(dllexport)
    #else
        #define MILLION_ETCD_API __declspec(dllimport)
    #endif
#else
    #define MILLION_ETCD_API
#endif

// 前向声明，避免直接包含etcd/api.h造成循环依赖
namespace million {
namespace etcd {
    class EtcdApi;
}
}

namespace million {
namespace etcd {

constexpr const char* kEtcdServiceName = "EtcdService";

// Etcd 键值对存在性检查请求
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdKeyExistReq, (std::string) key);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdKeyExistResp, (bool) exist);

// Etcd 设置键值对请求
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdSetReq, (std::string) key, (std::string) value);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdSetResp, (bool) success);

// Etcd 获取键值请求
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdGetReq, (std::string) key);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdGetResp, (std::optional<std::string>) value);

// Etcd 删除键请求
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdDelReq, (std::string) key);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdDelResp, (bool) success);

// Etcd 获取带前缀的所有键值对请求
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdGetAllReq, (std::string) prefix);
using StringMap = std::map<std::string, std::string>;
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdGetAllResp, (StringMap) kvs);

// Etcd 监听键变化请求
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdWatchReq, (std::string) key);
MILLION_MESSAGE_DEFINE_EMPTY(MILLION_ETCD_API, EtcdWatchResp);

// 配置相关消息
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdConfigLoadReq, (std::string) config_key);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdConfigLoadResp, (std::optional<std::string>) config_value);

MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdConfigWatchReq, (std::string) config_key);
MILLION_MESSAGE_DEFINE_EMPTY(MILLION_ETCD_API, EtcdConfigWatchResp);

// 服务注册与发现相关消息
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdServiceRegisterReq, (std::string) service_name, (std::string) service_address, (int) ttl);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdServiceRegisterResp, (bool) success);

MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdServiceUnregisterReq, (std::string) service_name, (std::string) service_address);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdServiceUnregisterResp, (bool) success);

MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdServiceDiscoverReq, (std::string) service_name);
MILLION_MESSAGE_DEFINE(MILLION_ETCD_API, EtcdServiceDiscoverResp, (std::vector<std::string>) service_addresses);

} // namespace etcd
} // namespace million