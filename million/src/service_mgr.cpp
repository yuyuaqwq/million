#include <limits> 

#include "service_mgr.h"

#include "million.h"
#include "service.h"
#include "million_msg.h"

namespace million {

ServiceMgr::ServiceMgr(Million* million)
    : million_(million) {}

ServiceMgr::~ServiceMgr() = default;

void ServiceMgr::Stop() {
    {
        auto lock = std::unique_lock(service_queue_mutex_);
        run_ = false;
    }
    service_queue_cv_.notify_one();
    for (auto& service : services_) {
        service->Stop();
    }
}

ServiceId ServiceMgr::AllocServiceId() {
    auto id = ++service_id_;
    if (id == std::numeric_limits<uint64_t>::max()) {
        throw std::overflow_error("Service ID overflow: no more IDs available.");
    }
    return id;
}

std::optional<ServiceHandle> ServiceMgr::AddService(std::unique_ptr<IService> iservice) {
    decltype(services_)::iterator iter;
    auto service_shared = std::make_shared<Service>(this, std::move(iservice));
    auto handle = ServiceHandle(service_shared);
    service_shared->iservice().set_service_handle(handle);
    auto service_ptr = service_shared.get();
    if (!service_shared->iservice().OnInit()) {
        return std::nullopt;
    }
    {
        auto lock = std::lock_guard(services_mutex_);
        services_.emplace_back(std::move(service_shared));
        iter = --services_.end();
    }
    service_ptr->set_iter(iter);
    handle.service()->Start();
    return handle;
}

void ServiceMgr::StopService(const ServiceHandle& handle) {
    handle.service()->Stop();
}

void ServiceMgr::DeleteService(Service* service) {
    {
        auto lock = std::lock_guard(services_mutex_);
        services_.erase(service->iter());
    }
}

void ServiceMgr::PushService(Service* service) {
    if (service->HasSeparateWorker()) {
        return;
    }
    bool has_push = false;
    {
        auto lock = std::lock_guard(service_queue_mutex_);
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

Service* ServiceMgr::PopService() {
    auto lock = std::unique_lock(service_queue_mutex_);
    while (run_ && service_queue_.empty()) {
        service_queue_cv_.wait(lock);
    }
    if (!run_) return nullptr;
    // std::cout << "wake up" << std::endl;
    auto* service = service_queue_.front();
    assert(service);
    service_queue_.pop();
    return service;
}

bool ServiceMgr::SetServiceName(const ServiceHandle& handle, const ServiceName& name) {
    auto lock = std::lock_guard(name_map_mutex_);
    auto res = name_map_.emplace(name, handle);
    return res.second;
}

std::optional<ServiceHandle> ServiceMgr::GetServiceByName(const ServiceName& name) {
    auto lock = std::lock_guard(name_map_mutex_);
    auto iter = name_map_.find(name);
    if (iter == name_map_.end()) {
        return std::nullopt;
    }
    return iter->second;
}

bool ServiceMgr::SetServiceId(const ServiceHandle& handle, ServiceId id) {
    auto lock = std::lock_guard(id_map_mutex_);
    auto res = id_map_.emplace(id, handle);
    return res.second;
}

std::optional<ServiceHandle> ServiceMgr::GetServiceById(ServiceId id) {
    auto lock = std::lock_guard(id_map_mutex_);
    auto iter = id_map_.find(id);
    if (iter == id_map_.end()) {
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