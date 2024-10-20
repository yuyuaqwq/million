#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <list>
#include <queue>

#include <million/msg_def.h>

#include "service.h"

namespace million {

class Million;
class ServiceMgr {
public:
    ServiceMgr(Million* million);
    ~ServiceMgr();

    ServiceHandle AddService(std::unique_ptr<IService> service);
    void RemoveService(ServiceHandle handle);

    void PushService(Service* service);
    Service& PopService();

    void SetServiceCodeName(ServiceHandle handle, const ServiceCodeName& code_name);
    std::optional<ServiceHandle> GetServiceByCodeNum(const ServiceCodeName& code_name);

    SessionId Send(ServiceHandle sender, ServiceHandle target, MsgUnique msg);
    
    Million* million() const { return million_; }

private:
    Million* million_;

    std::mutex services_mutex_;
    std::list<std::unique_ptr<Service>> services_;
    
    std::mutex service_code_name_map_mutex_;
    std::unordered_map<ServiceCodeName, ServiceHandle> service_code_name_map_;

    std::mutex service_queue_mutex_;
    std::queue<Service*> service_queue_;
    std::condition_variable service_queue_cv_;
};

} // namespace million