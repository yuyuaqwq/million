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

    ServiceId AllocServiceId();

    std::optional<ServiceHandle> AddService(std::unique_ptr<IService> service);
    void StopService(const ServiceHandle& handle);
    void DeleteService(Service* service);

    void PushService(Service* service);
    Service& PopService();

    bool SetServiceName(const ServiceHandle& handle, const ServiceName& name);
    std::optional<ServiceHandle> GetServiceByName(const ServiceName& name);

    bool SetServiceId(const ServiceHandle& handle, ServiceId id);
    std::optional<ServiceHandle> GetServiceById(ServiceId id);

    SessionId Send(const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg);
    
    Million* million() const { return million_; }

private:
    Million* million_;

    std::atomic<ServiceId> service_id_ = 0;

    std::mutex services_mutex_;
    std::list<std::shared_ptr<Service>> services_;
    
    std::mutex name_map_mutex_;
    std::unordered_map<ServiceName, ServiceHandle> name_map_;

    std::mutex id_map_mutex_;
    std::unordered_map<ServiceId, ServiceHandle> id_map_;

    std::mutex service_queue_mutex_;
    std::queue<Service*> service_queue_;
    std::condition_variable service_queue_cv_;
};

} // namespace million