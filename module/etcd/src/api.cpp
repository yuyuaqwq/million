#include <etcd/api.h>

namespace million {
namespace etcd {

bool EtcdApi::Init(IMillion* imillion, const std::string& endpoints) {
    // TODO: 实现初始化逻辑
    // 这里可以存储 endpoints 供后续使用
    return true;
}

Task<bool> EtcdApi::Set(IService* service, const ServiceHandle& etcd_service, const std::string& key, const std::string& value) {
    auto resp = co_await service->Call<EtcdSetReq, EtcdSetResp>(etcd_service, key, value);
    co_return resp->success;
}

Task<std::optional<std::string>> EtcdApi::Get(IService* service, const ServiceHandle& etcd_service, const std::string& key) {
    auto resp = co_await service->Call<EtcdGetReq, EtcdGetResp>(etcd_service, key);
    co_return std::move(resp->value);
}

Task<bool> EtcdApi::Del(IService* service, const ServiceHandle& etcd_service, const std::string& key) {
    auto resp = co_await service->Call<EtcdDelReq, EtcdDelResp>(etcd_service, key);
    co_return resp->success;
}

Task<std::map<std::string, std::string>> EtcdApi::GetAll(IService* service, const ServiceHandle& etcd_service, const std::string& prefix) {
    auto resp = co_await service->Call<EtcdGetAllReq, EtcdGetAllResp>(etcd_service, prefix);
    co_return std::move(resp->kvs);
}

Task<std::optional<std::string>> EtcdApi::LoadConfig(IService* service, const ServiceHandle& etcd_service, const std::string& config_key) {
    auto resp = co_await service->Call<EtcdConfigLoadReq, EtcdConfigLoadResp>(etcd_service, config_key);
    co_return std::move(resp->config_value);
}

Task<bool> EtcdApi::WatchConfig(IService* service, const ServiceHandle& etcd_service, const std::string& config_key, 
                                std::function<void(const std::string&)> callback) {
    // TODO: 实现配置监听逻辑
    // 这需要在服务端实现相应的监听机制
    auto resp = co_await service->Call<EtcdConfigWatchReq, EtcdConfigWatchResp>(etcd_service, config_key);
    co_return true;
}

Task<bool> EtcdApi::RegisterService(IService* service, const ServiceHandle& etcd_service, 
                                    const std::string& service_name, const std::string& service_address, int ttl) {
    auto resp = co_await service->Call<EtcdServiceRegisterReq, EtcdServiceRegisterResp>(etcd_service, service_name, service_address, ttl);
    co_return resp->success;
}

Task<bool> EtcdApi::UnregisterService(IService* service, const ServiceHandle& etcd_service, 
                                      const std::string& service_name, const std::string& service_address) {
    auto resp = co_await service->Call<EtcdServiceUnregisterReq, EtcdServiceUnregisterResp>(etcd_service, service_name, service_address);
    co_return resp->success;
}

Task<std::vector<std::string>> EtcdApi::DiscoverService(IService* service, const ServiceHandle& etcd_service, 
                                                        const std::string& service_name) {
    auto resp = co_await service->Call<EtcdServiceDiscoverReq, EtcdServiceDiscoverResp>(etcd_service, service_name);
    co_return std::move(resp->service_addresses);
}

} // namespace etcd
} // namespace million