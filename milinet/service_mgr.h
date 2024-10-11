#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <list>
#include <queue>

#include "milinet/msg_def.h"
#include "milinet/service.h"

namespace milinet {

class Milinet;
class ServiceMgr {
public:
    ServiceMgr(Milinet* milinet);
    ~ServiceMgr();

    ServiceHandle AddService(std::unique_ptr<IService> service);
    void RemoveService(ServiceHandle handle);

    void PushService(Service* service);
    Service& PopService();

    SessionId Send(ServiceHandle target, MsgUnique msg);
    
    Milinet* milinet() const { return milinet_; }

private:
    Milinet* milinet_;

    std::mutex services_mutex_;
    std::list<std::unique_ptr<Service>> services_;
    
    std::mutex service_queue_mutex_;
    std::queue<Service*> service_queue_;
    std::condition_variable service_queue_cv_;
};

} // namespace milinet