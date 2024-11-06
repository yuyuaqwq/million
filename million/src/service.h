#pragma once

#include <cstdint>

#include <memory>
#include <mutex>
#include <queue>

#include <million/noncopyable.h>
#include <million/iservice.h>
#include <million/service_handle.h>
#include <million/imsg.h>

#include "msg_executor.h"

namespace million {
enum class ServiceState {
    kReady,
    kRunning,
    kStopping,
    kStop,
};

class ServiceMgr;
class Service : noncopyable {
public:
    Service(ServiceMgr* service_mgr, std::unique_ptr<IService> iservice);
    ~Service();

    bool IsStoping() const;
    void Stop();

    void PushMsg(MsgUnique msg);
    std::optional<MsgUnique> PopMsg();
    bool MsgQueueEmpty();

    bool ProcessMsg();
    void ProcessMsgs(size_t count);

    ServiceState state() { return state_; }
    bool in_queue() const { return in_queue_; }
    void set_in_queue(bool in_queue) { in_queue_ = in_queue; }
    IService& iservice() const { assert(iservice_); return *iservice_; }
    const ServiceHandle& service_handle() const { return iservice_->service_handle(); }
    auto iter() const { return iter_; }
    void set_iter(std::list<std::shared_ptr<Service>>::iterator iter) { iter_ = iter; }
    ServiceMgr* service_mgr() const { return service_mgr_; }

private:
    ServiceMgr* service_mgr_;
    std::list<std::shared_ptr<Service>>::iterator iter_;

    std::mutex msgs_mutex_;
    std::queue<MsgUnique> msgs_;

    MsgExecutor excutor_;

    bool in_queue_ = false;
    ServiceState state_ = ServiceState::kReady;

    std::unique_ptr<IService> iservice_;
};

} // namespace million