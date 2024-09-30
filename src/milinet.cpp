#include "milinet/milinet.h"

#include <cassert>

#include <iostream>

#include "milinet/service.h"
#include "milinet/worker.h"

namespace milinet {

Milinet::Milinet(size_t work_thread_num) : service_id_(0) {
    workers_.reserve(work_thread_num);
    for (size_t i = 0; i < work_thread_num; ++i) {
        workers_.emplace_back(std::make_unique<Worker>(this));
    }
}

Milinet::~Milinet() = default;

void Milinet::Start() {
    for (auto& worker : workers_) {
        worker->Start();
    }
}

ServiceId Milinet::AllocServiceId() {
    auto id = ++service_id_;
    if (id == 0) {
        std::runtime_error("service id rolled back.");
    }
    return id;
}

Service& Milinet::AddService(std::unique_ptr<Service> service) {
    auto id = service->id();
    auto ptr = service.get();
    {
        std::lock_guard guard(service_map_mutex_);
        service_map_.emplace(std::make_pair(id, std::move(service)));
    }
    return *ptr;
}

void Milinet::RemoveService(ServiceId id) {
    {
        std::lock_guard guard(service_map_mutex_);
        service_map_.erase(id);
    }
}

Service* Milinet::GetService(ServiceId id) {
    decltype(service_map_)::iterator iter;
    {
        std::lock_guard guard(service_map_mutex_);
        iter = service_map_.find(id);
        if (iter == service_map_.end()){
            return nullptr;
        }
    }
    return iter->second.get();
}

void Milinet::PushService(Service* service) {
    bool has_push = false;
    {
        std::lock_guard guard(service_queue_mutex_);
        if (!service->in_queue()){
            service_queue_.emplace(service);
            service->set_in_queue(true);
            has_push = true;
        }
    }
    //if (has_push) {
        service_queue_cv_.notify_one();
    //}
}

Service& Milinet::PopService() {
    std::unique_lock guard(service_queue_mutex_);
    while (service_queue_.empty()) {
        service_queue_cv_.wait(guard);
    }
    std::cout << "wake up" << std::endl;
    auto* service = service_queue_.front();
    assert(service);
    service_queue_.pop();
    service->set_in_queue(false);
    return *service;
}

SessionId Milinet::AllocSessionId() {
    auto id = ++session_id_;
    if (id == 0) {
        std::runtime_error("session id rolled back.");
    }
    return id;
}

void Milinet::Send(ServiceId target_id, MsgUnique msg) {
    assert(msg);
    auto* service = GetService(target_id);
    if (!service) return;
    Send(service, std::move(msg));
}

void Milinet::Send(Service* target, MsgUnique msg) {
    target->PushMsg(std::move(msg));
    PushService(target);
}

} //namespace milinet