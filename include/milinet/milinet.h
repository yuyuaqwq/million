#pragma once

#include <cstdint>

#include <memory>

#include "milinet/noncopyable.h"
#include "milinet/service_mgr.h"
#include "milinet/msg_mgr.h"
#include "milinet/module_mgr.h"
#include "milinet/worker_mgr.h"

namespace milinet {

class ConfigException : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message)
        : std::runtime_error("YAML config error: " + message) {}
};

// todo：分个Impl类，只暴露必要的接口

class Milinet : noncopyable {
public:
    Milinet(std::string_view config_path);
    ~Milinet();

    void Init();
    void Start();

    template <typename ServiceT, typename ...Args>
    Service& CreateService(Args&&... args) {
        return service_mgr_->AddService(service_mgr_->MakeService<ServiceT>(std::forward<Args>(args)...));
    }

    template <typename MsgT, typename ...Args>
    SessionId Send(ServiceId service_id, Args&&... args) {
        return service_mgr_->Send(service_id, msg_mgr_->MakeMsg<MsgT>(std::forward<Args>(args)...));
    }

    auto& service_mgr() { assert(service_mgr_); return *service_mgr_; }
    auto& msg_mgr() { assert(service_mgr_); return *msg_mgr_; }
    auto& module_mgr() { assert(module_mgr_); return *module_mgr_; }
    auto& worker_mgr() { assert(worker_mgr_); return *worker_mgr_; }

private:
    std::unique_ptr<ServiceMgr> service_mgr_;
    std::unique_ptr<MsgMgr> msg_mgr_;
    std::unique_ptr<ModuleMgr> module_mgr_;
    std::unique_ptr<WorkerMgr> worker_mgr_;
};

template <typename MsgT, typename ...Args>
SessionId ServiceMgr::Send(ServiceId service_id, Args&&... args) {
    return milinet_->Send<MsgT>(service_id, std::forward<Args>(args)...);
}

} // namespace milinet