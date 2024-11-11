#pragma once

#include <cstdint>

#include <functional>
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
    kStop,
};

class ServiceMgr;
class Service : noncopyable {
public:
    Service(ServiceMgr* service_mgr, std::unique_ptr<IService> iservice);
    ~Service();

    bool IsStop() const;

    void PushMsg(MsgUnique msg);
    std::optional<MsgUnique> PopMsg();
    bool MsgQueueIsEmpty();

    void ProcessMsg(MsgUnique msg);
    void ProcessMsgs(size_t count);

    void EnableSeparateWorker();
    bool HasSeparateWorker() const;

    ServiceMgr* service_mgr() const { return service_mgr_; }
    
    auto iter() const { return iter_; }
    void set_iter(std::list<std::shared_ptr<Service>>::iterator iter) { iter_ = iter; }

    ServiceState state() { return state_; }

    bool in_queue() const { return in_queue_; }
    void set_in_queue(bool in_queue) { in_queue_ = in_queue; }

    IService& iservice() const { assert(iservice_); return *iservice_; }

    const ServiceHandle& service_handle() const { return iservice_->service_handle(); }

private:
    void SeparateThreadHandle();
    std::optional<MsgUnique> PopMsgWithLock();

private:
    ServiceMgr* service_mgr_;
    std::list<std::shared_ptr<Service>>::iterator iter_;

    ServiceState state_ = ServiceState::kReady;

    bool in_queue_ = false;

    std::mutex msgs_mutex_;
    std::queue<MsgUnique> msgs_;

    TaskExecutor excutor_;

    std::unique_ptr<IService> iservice_;

    struct SeparateWorker {
        std::thread thread;
        std::condition_variable cv;
        SeparateWorker(const std::function<void()>& func) : thread(func) { thread.detach(); }
    };
    std::unique_ptr<SeparateWorker> separate_worker_;
};

} // namespace million