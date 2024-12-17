#pragma once

#include <cstdint>

#include <functional>
#include <memory>
#include <mutex>
#include <queue>

#include <million/noncopyable.h>
#include <million/iservice.h>
#include <million/service_handle.h>
#include <million/msg.h>

#include "task_executor.h"

namespace million {

class ServiceMgr;
class Service : noncopyable {
public:
    struct MsgElement{
        ServiceHandle sender;
        SessionId session_id;
        MsgUnique msg;
    };

public:
    Service(ServiceMgr* service_mgr, std::unique_ptr<IService> iservice);
    ~Service();

    void PushMsg(const ServiceHandle& sender, SessionId session_id, MsgUnique msg);
    std::optional<MsgElement> PopMsg();
    bool MsgQueueIsEmpty();

    void ProcessMsg(MsgElement msg);
    void ProcessMsgs(size_t count);

    void EnableSeparateWorker();
    bool HasSeparateWorker() const;

    ServiceMgr* service_mgr() const { return service_mgr_; }
    
    auto iter() const { return iter_; }
    void set_iter(std::list<std::shared_ptr<Service>>::iterator iter) { iter_ = iter; }

    bool in_queue() const { return in_queue_; }
    void set_in_queue(bool in_queue) { in_queue_ = in_queue; }

    IService& iservice() const { assert(iservice_); return *iservice_; }

    const ServiceHandle& service_handle() const { return iservice_->service_handle(); }

    // ‘›«“≤ª π”√
    SessionId Start();
    SessionId Stop();
    SessionId Exit();

    bool IsReady() const;
    bool IsStarting() const;
    bool IsRunning() const;
    bool IsStop() const;
    bool IsExit() const;

private:
    void SeparateThreadHandle();
    std::optional<Service::MsgElement> PopMsgWithLock();

private:
    ServiceMgr* service_mgr_;
    std::list<std::shared_ptr<Service>>::iterator iter_;

    // std::optional<Task<>> on_start_task_;
    enum State {
        kReady,
        kStarting,
        kRunning,
        kStop,
        kExit,
    };
    State state_ = kReady;

    bool in_queue_ = false;

    std::mutex msgs_mutex_;
    std::queue<MsgElement> msgs_;

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