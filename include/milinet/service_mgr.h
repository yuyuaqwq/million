#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <queue>

#include "milinet/msg_def.h"
#include "milinet/service.h"

namespace milinet {

class Milinet;
class ServiceMgr {
public:
    ServiceMgr(Milinet* milinet);
    ~ServiceMgr();

    ServiceId AllocServiceId();

    template <typename ServiceT, typename ...Args>
    std::unique_ptr<ServiceT> MakeService(Args&&... args) {
        auto id = AllocServiceId();
        return std::make_unique<ServiceT>(this, id, std::forward<Args>(args)...);
    }

    Service& AddService(std::unique_ptr<Service> service);
    void RemoveService(ServiceId service_id);
    Service* GetService(ServiceId service_id);

    void PushService(Service* service);
    Service& PopService();

    SessionId Send(Service* service, MsgUnique msg);
    SessionId Send(ServiceId service_id, MsgUnique msg);
    
    template <typename MsgT, typename ...Args>
    SessionId Send(ServiceId service_id, Args&&... args);

private:
    Milinet* milinet_;

    std::atomic<ServiceId> service_id_ = 0;

    std::mutex service_map_mutex_;
    std::unordered_map<ServiceId, std::unique_ptr<Service>> service_map_;
    
    std::mutex service_queue_mutex_;
    std::queue<Service*> service_queue_;
    std::condition_variable service_queue_cv_;
};

template <typename MsgT, typename ...Args>
SessionId Service::Send(ServiceId service_id, Args&&... args) {
    return mgr_->Send<MsgT>(service_id, std::forward<Args>(args)...);
}

} // namespace milinet