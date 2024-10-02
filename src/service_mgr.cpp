#include "milinet/service_mgr.h"

#include "milinet/service.h"

namespace milinet {

ServiceMgr::ServiceMgr(Milinet* milinet) 
    : milinet_(milinet) {}

ServiceMgr::~ServiceMgr() = default;

ServiceId ServiceMgr::AllocServiceId() {
    auto id = ++service_id_;
    if (id == 0) {
        std::runtime_error("service id rolled back.");
    }
    return id;
}

Service& ServiceMgr::AddService(std::unique_ptr<Service> service) {
    auto id = service->service_id();
    auto ptr = service.get();
    {
        std::lock_guard guard(service_map_mutex_);
        service_map_.emplace(std::make_pair(id, std::move(service)));
    }
    return *ptr;
}

void ServiceMgr::RemoveService(ServiceId service_id) {
    {
        std::lock_guard guard(service_map_mutex_);
        service_map_.erase(service_id);
    }
}

Service* ServiceMgr::GetService(ServiceId service_id) {
    decltype(service_map_)::iterator iter;
    {
        std::lock_guard guard(service_map_mutex_);
        iter = service_map_.find(service_id);
        if (iter == service_map_.end()){
            return nullptr;
        }
    }
    return iter->second.get();
}

void ServiceMgr::PushService(Service* service) {
    bool has_push = false;
    {
        std::lock_guard guard(service_queue_mutex_);
        if (!service->in_queue()){
            service_queue_.emplace(service);
            service->set_in_queue(true);
            has_push = true;
        }
    }
    if (has_push) {
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
    service->set_in_queue(false);
    return *service;
}

SessionId ServiceMgr::Send(Service* target, MsgUnique msg) {
    auto id = msg->session_id();
    target->PushMsg(std::move(msg));
    PushService(target);
    return id;
}

SessionId ServiceMgr::Send(ServiceId target_id, MsgUnique msg) {
    assert(msg);
    auto* service = GetService(target_id);
    if (!service) return kSessionIdInvalid;
    return Send(service, std::move(msg));
}

} // namespace milinet