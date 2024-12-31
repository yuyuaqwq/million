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
        ServiceShared sender;
        SessionId session_id;
        MsgUnique msg;
    };

public:
    Service(ServiceMgr* service_mgr, std::unique_ptr<IService> iservice);
    ~Service();

    bool PushMsg(const ServiceShared& sender, SessionId session_id, MsgUnique msg);
    std::optional<MsgElement> PopMsg();
    bool MsgQueueIsEmpty();

    void ProcessMsg(MsgElement msg);
    void ProcessMsgs(size_t count);

    bool TaskExecutorIsEmpty() const;

    void EnableSeparateWorker();
    bool HasSeparateWorker() const;

    ServiceMgr* service_mgr() const { return service_mgr_; }
    
    ServiceShared shared() const { return *iter_; }

    auto iter() const { return iter_; }
    void set_iter(std::list<std::shared_ptr<Service>>::iterator iter) { iter_ = iter; }

    bool in_queue() const { return in_queue_; }
    void set_in_queue(bool in_queue) { in_queue_ = in_queue; }

    IService& iservice() const { assert(iservice_); return *iservice_; }

    std::optional<SessionId> Start();
    std::optional<SessionId> Stop();
    std::optional<SessionId> Exit();

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
    std::list<ServiceShared>::iterator iter_;

    // std::optional<Task<>> on_start_task_;
    enum State {
        // 已就绪，可以开启服务
        kReady,
        // 开启中，只能处理OnStart相关的消息及调度OnStart协程
        kStarting,
        // 运行中，可以接收及处理任何消息、调度已有协程
        kRunning,
        // 关闭后，不会开启新协程，不会调度已有协程，会触发已有协程的超时
        kStop,
        // 退出后，不会开启新协程，不会调度已有协程，不会触发已有协程的超时(希望执行完所有协程再退出，则需要在Stop状态等待所有协程处理完毕，即使用TaskExecutorIsEmpty)
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