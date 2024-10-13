#include "million/service_mgr.h"

#include "million/service.h"

namespace million {

ServiceMgr::ServiceMgr(Million* million)
    : million_(million) {}

ServiceMgr::~ServiceMgr() = default;

ServiceHandle ServiceMgr::AddService(std::unique_ptr<IService> iservice) {
    decltype(services_)::iterator iter;
    auto service = std::make_unique<Service>(std::move(iservice));
    {
        std::lock_guard guard(services_mutex_);
        services_.emplace_back(std::move(service));
        iter = --services_.end();
    }
    auto handle = ServiceHandle(iter);
    handle.service().iservice().set_service_handle(handle);
    handle.service().iservice().OnInit();
    return handle;
}

void ServiceMgr::RemoveService(ServiceHandle handle) {
    {
        std::lock_guard guard(services_mutex_);
        services_.erase(handle.iter());
    }
}

void ServiceMgr::PushService(Service* service) {
    if (!service->in_queue()){
        {
            std::lock_guard guard(service_queue_mutex_);
            service_queue_.emplace(service);
            // set为false的时机，在ProcessMsg完成后设置
            // 避免当前Service在ProcessMsg时，被通过Send投递消息
            // 再次将当前ServicePush到队列，使得当前Service被多个Work线程持有
            service->set_in_queue(true);
        }
        service_queue_cv_.notify_one();
    }
}

Service& ServiceMgr::PopService() {
    std::unique_lock guard(service_queue_mutex_);
    while (service_queue_.empty()) {
        service_queue_cv_.wait(guard);
    }
    // std::cout << "wake up" << std::endl;
    auto* service = service_queue_.front();
    assert(service);
    service_queue_.pop();
    return *service;
}

SessionId ServiceMgr::Send(ServiceHandle target, MsgUnique msg) {
    auto id = msg->session_id();
    auto& service = target.service();
    service.PushMsg(std::move(msg));
    PushService(&service);
    return id;
}

} // namespace million