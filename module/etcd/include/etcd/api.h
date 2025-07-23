#pragma once

#include <string>
#include <optional>
#include <map>
#include <vector>
#include <functional>

#include <million/imillion.h>

#include <etcd/etcd.h>

namespace million {
namespace etcd {

class MILLION_ETCD_API EtcdApi {
public:
    // 初始化 etcd 客户端
    static bool Init(IMillion* imillion, const std::string& endpoints);
    
    // 设置键值对
    static Task<bool> Set(IService* service, const ServiceHandle& etcd_service, const std::string& key, const std::string& value);
    
    // 获取键值
    static Task<std::optional<std::string>> Get(IService* service, const ServiceHandle& etcd_service, const std::string& key);
    
    // 删除键
    static Task<bool> Del(IService* service, const ServiceHandle& etcd_service, const std::string& key);
    
    // 获取所有具有指定前缀的键值对
    static Task<std::map<std::string, std::string>> GetAll(IService* service, const ServiceHandle& etcd_service, const std::string& prefix);
    
    // 配置相关接口
    static Task<std::optional<std::string>> LoadConfig(IService* service, const ServiceHandle& etcd_service, const std::string& config_key);
    static Task<bool> WatchConfig(IService* service, const ServiceHandle& etcd_service, const std::string& config_key, 
                                  std::function<void(const std::string&)> callback);
    
    // 服务注册与发现接口
    static Task<bool> RegisterService(IService* service, const ServiceHandle& etcd_service, 
                                      const std::string& service_name, const std::string& service_address, int ttl = 10);
    static Task<bool> UnregisterService(IService* service, const ServiceHandle& etcd_service, 
                                        const std::string& service_name, const std::string& service_address);
    static Task<std::vector<std::string>> DiscoverService(IService* service, const ServiceHandle& etcd_service, 
                                                          const std::string& service_name);
};

} // namespace etcd
} // namespace million