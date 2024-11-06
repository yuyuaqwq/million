#include "service_mgr.h"

#include "million.h"
#include "service.h"
#include "million_msg.h"

namespace million {

ServiceMgr::ServiceMgr(Million* million)
    : million_(million) {}

ServiceMgr::~ServiceMgr() = default;

ServiceHandle ServiceMgr::AddService(std::unique_ptr<IService> iservice) {
    decltype(services_)::iterator iter;
    auto service_shared = std::make_shared<Service>(this, std::move(iservice));
    auto handle = ServiceHandle(service_shared);
    service_shared->iservice().set_service_handle(handle);
    auto service_ptr = service_shared.get();
    {
        auto lock = std::lock_guard(services_mutex_);
        services_.emplace_back(std::move(service_shared));
        iter = --services_.end();
    }
    service_ptr->set_iter(iter);
    million_->Send<MillionServiceInitMsg>(handle, handle);
    return handle;
}

void ServiceMgr::DeleteService(const ServiceHandle& handle) {
    million_->Send<MillionServiceExitMsg>(handle, handle);
}

void ServiceMgr::DeleteService(Service* service) {
    {
        auto lock = std::lock_guard(services_mutex_);
        services_.erase(service->iter());
    }
}


void ServiceMgr::PushService(Service* service) {
    bool has_push = false;
    {
        std::lock_guard guard(service_queue_mutex_);
        if (!service->in_queue()) {
            service_queue_.emplace(service);
            // set为false的时机，在ProcessMsg完成后设置
            // 避免当前Service同时被多个Work线程持有并ProcessMsg
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
    return *service;
}

void ServiceMgr::SetServiceCodeName(const ServiceHandle& handle, const ServiceCodeName& code_name) {
    std::lock_guard guard(service_code_name_map_mutex_);
    service_code_name_map_.insert(std::make_pair(code_name, handle));
}

std::optional<ServiceHandle> ServiceMgr::GetServiceByCodeNum(const ServiceCodeName& code_name) {
    std::lock_guard guard(service_code_name_map_mutex_);
    auto iter = service_code_name_map_.find(code_name);
    if (iter == service_code_name_map_.end()) {
        return std::nullopt;
    }
    return iter->second;
}

SessionId ServiceMgr::Send(const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg) {
    msg->set_sender(sender);
    auto id = msg->session_id();
    auto service = target.service();
    if (!service) {
        return kSessionIdInvalid;
    }
    service->PushMsg(std::move(msg));
    PushService(service);
    return id;
}

} // namespace million