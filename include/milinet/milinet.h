#pragma once

#include <cstdint>

#include <memory>

#include "milinet/imilinet.h"
#include "milinet/service_mgr.h"
#include "milinet/msg_mgr.h"

namespace milinet {

class ConfigException : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message)
        : std::runtime_error("YAML config error: " + message) {}
};

class ModuleMgr;
class WorkerMgr;
class Milinet : public IMilinet {
public:
    Milinet(std::string_view config_path);
    ~Milinet();

    void Init();
    void Start();

    virtual ServiceId CreateService(std::unique_ptr<IService> iservice) override {
        return service_mgr_->AddService(std::move(iservice));
    }

    virtual SessionId Send(ServiceId service_id, MsgUnique msg) override {
        msg->set_session_id(msg_mgr_->AllocSessionId());
        return service_mgr_->Send(service_id, std::move(msg));
    }

    template <typename MsgT, typename ...Args>
    SessionId Send(ServiceId service_id, Args&&... args) {
        return Send(service_id, msg_mgr_->MakeMsg<MsgT>(std::forward<Args>(args)...));
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

} // namespace milinet