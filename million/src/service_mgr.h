#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <list>
#include <queue>

#include "service_core.h"

namespace million {

MILLION_MESSAGE_DEFINE_NONCOPYABLE(, ServiceStartMsg, (MessagePointer) with_msg);
MILLION_MESSAGE_DEFINE_NONCOPYABLE(, ServiceStopMsg, (MessagePointer) with_msg);
MILLION_MESSAGE_DEFINE_EMPTY(, ServiceExitMsg);

class Million;
class ServiceMgr {
public:
    ServiceMgr(Million* million);
    ~ServiceMgr();

    void Stop();

    ServiceId AllocServiceId();

    std::optional<ServiceShared> AddService(std::unique_ptr<IService> service);
    void DeleteService(ServiceCore* service);

    std::optional<SessionId> StartService(const ServiceShared& service, MessagePointer with_msg);
    std::optional<SessionId> StopService(const ServiceShared& service, MessagePointer with_msg);
    std::optional<SessionId> ExitService(const ServiceShared& service);

    void PushService(ServiceCore* service);
    ServiceCore* PopService();

    bool SetServiceName(const ServiceShared& handle, const ServiceName& name);
    std::optional<ServiceShared> GetServiceByName(const ServiceName& name);

    bool SetServiceId(const ServiceShared& handle, ServiceId id);
    std::optional<ServiceShared> GetServiceById(ServiceId id);

    bool Send(const ServiceShared& sender, const ServiceShared& target, SessionId session_id, MessagePointer msg);
    
    Million& million() const { return *million_; }

private:
    Million* million_;

    std::atomic<ServiceId> service_id_ = 0;

    std::mutex services_mutex_;
    std::list<ServiceShared> services_;
    
    std::mutex name_map_mutex_;
    std::unordered_map<ServiceName, std::list<ServiceShared>::iterator> name_map_;

    std::mutex id_map_mutex_;
    std::unordered_map<ServiceId, std::list<ServiceShared>::iterator> id_map_;

    std::mutex service_queue_mutex_;
    bool run_ = true;
    std::queue<ServiceCore*> service_queue_;
    std::condition_variable service_queue_cv_;
};

} // namespace million