#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <list>
#include <queue>

#include "service.h"

namespace million {

class Million;
class ServiceMgr {
public:
    ServiceMgr(Million* million);
    ~ServiceMgr();

    std::optional<ServiceHandle> AddService(std::unique_ptr<IService> service);
    void DeleteService(ServiceHandle&& handle);
    void DeleteService(Service* service);

    void PushService(Service* service);
    Service& PopService();

    bool SetServiceUniqueName(const ServiceHandle& handle, const ServiceUniqueName& unique_name);
    std::optional<ServiceHandle> GetServiceByUniqueNum(const ServiceUniqueName& unique_name);

    SessionId Send(const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg);
    
    Million* million() const { return million_; }


private:
    Million* million_;

    std::mutex services_mutex_;
    std::list<std::shared_ptr<Service>> services_;
    
    std::mutex unique_name_map_mutex_;
    std::unordered_map<ServiceUniqueName, ServiceHandle> unique_name_map_;

    std::mutex service_queue_mutex_;
    std::queue<Service*> service_queue_;
    std::condition_variable service_queue_cv_;
};

} // namespace million