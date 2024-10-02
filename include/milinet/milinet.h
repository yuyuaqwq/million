#pragma once

#include <cstdint>

#include <vector>
#include <memory>

#include "noncopyable.h"

#include "milinet/msg_mgr.h"
#include "milinet/service_mgr.h"

namespace milinet {

// todo：分个Impl类，只暴露必要的接口

class Worker;
class Milinet : noncopyable {
public:
    Milinet(std::string_view config_path);
    ~Milinet();

    void Start();

    template <typename ServiceT, typename ...Args>
    Service& CreateService(Args&&... args) {
        return service_mgr_.AddService(service_mgr_.MakeService<ServiceT>(std::forward<Args>(args)...));
    }

    template <typename MsgT, typename ...Args>
    SessionId Send(ServiceId service_id, Args&&... args) {
        return service_mgr_.Send(service_id, msg_mgr_.MakeMsg<MsgT>(std::forward<Args>(args)...));
    }

    ServiceMgr& service_mgr() { return service_mgr_; }
    MsgMgr& msg_mgr() { return msg_mgr_; }

private:
    ServiceMgr service_mgr_;
    MsgMgr msg_mgr_;

    inline static size_t kWorkThreadNum = std::thread::hardware_concurrency();
    std::vector<std::unique_ptr<Worker>> workers_;
};

template <typename MsgT, typename ...Args>
SessionId ServiceMgr::Send(ServiceId service_id, Args&&... args) {
    return milinet_->Send<MsgT>(service_id, std::forward<Args>(args)...);
}

} // namespace milinet