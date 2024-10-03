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

    ServiceId AddService(std::unique_ptr<IService> service);
    void RemoveService(ServiceId service_id);
    Service* GetService(ServiceId service_id);

    void PushService(Service* service);
    Service& PopService();

    SessionId Send(ServiceId target_id, MsgUnique msg);
    SessionId Send(Service* target, MsgUnique msg);
    
    Milinet* milinet() const { return milinet_; }

private:
    Milinet* milinet_;

    std::atomic<ServiceId> service_id_ = 0;

    std::mutex service_map_mutex_;
    std::unordered_map<ServiceId, std::unique_ptr<Service>> service_map_;
    
    std::mutex service_queue_mutex_;
    std::queue<Service*> service_queue_;
    std::condition_variable service_queue_cv_;
};

} // namespace milinet