#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <list>
#include <queue>

#include "service.h"

namespace million {

MILLION_MSG_DEFINE_EMPTY(, ServiceStartMsg);
MILLION_MSG_DEFINE_EMPTY(, ServiceStopMsg);
MILLION_MSG_DEFINE_EMPTY(, ServiceExitMsg);

class Million;
class ServiceMgr {
public:
    ServiceMgr(Million* million);
    ~ServiceMgr();

    void Stop();

    ServiceId AllocServiceId();

    std::optional<ServiceHandle> AddService(std::unique_ptr<IService> service, bool start);
    void DeleteService(Service* service);

    SessionId StartService(const ServiceHandle& handle);
    SessionId StopService(const ServiceHandle& handle);
    SessionId ExitService(const ServiceHandle& handle);

    void PushService(Service* service);
    Service* PopService();

    bool SetServiceName(const ServiceHandle& handle, const ServiceName& name);
    std::optional<ServiceHandle> GetServiceByName(const ServiceName& name);

    bool SetServiceId(const ServiceHandle& handle, ServiceId id);
    std::optional<ServiceHandle> GetServiceById(ServiceId id);

    bool Send(const ServiceHandle& sender, const ServiceHandle& target, SessionId session_id, MsgUnique msg);
    
    Million& million() const { return *million_; }

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
    bool run_ = true;
    std::queue<Service*> service_queue_;
    std::condition_variable service_queue_cv_;
};

} // namespace million