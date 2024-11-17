#include "service.h"

#include <cassert>

#include <million/iservice.h>

#include "million.h"
#include "service_mgr.h"
#include "session_monitor.h"
#include "million_msg.h"

namespace million {

Service::Service(ServiceMgr* service_mgr, std::unique_ptr<IService> iservice)
    : service_mgr_(service_mgr)
    , iservice_(std::move(iservice))
    , excutor_(this) {}

Service::~Service() = default;

bool Service::IsExit() const {
    return state_ == ServiceState::kExit;
}

void Service::PushMsg(MsgUnique msg) {
    {
        auto lock = std::lock_guard(msgs_mutex_);
        if (IsExit()) {
            // 服务退出，不再接收消息
            return;
        }
        msgs_.emplace(std::move(msg));
    }
    if (HasSeparateWorker()) {
        separate_worker_->cv.notify_one();
    }
}

std::optional<MsgUnique> Service::PopMsg() {
    auto lock = std::lock_guard(msgs_mutex_);
    return PopMsgWithLock();
}

bool Service::MsgQueueIsEmpty() {
    std::lock_guard guard(msgs_mutex_);
    return msgs_.empty();
}

void Service::ProcessMsg(MsgUnique msg) {
    if (msg->type() == MillionServiceStartMsg::kType) {
        task_.emplace(iservice_->OnStart());
        if (!task_->coroutine.done()) {
            // OnStart未完成，等待处理完成
            auto id = task_->coroutine.promise().session_awaiter()->waiting_session();
            service_mgr()->million()->session_monitor().AddSession(service_handle(), id);
        }
        else {
            task_ = std::nullopt;
            state_ = ServiceState::kRunning;
        }
        return;
    }
    else if (msg->type() == MillionServiceStopMsg::kType) {
        // if (state_ == ServiceState::kReady) {
            // OnStart未完成，直接退出
            iservice_->OnExit();
            state_ = ServiceState::kExit;
            return;
        // }
        // 等待OnStop完成
        return;
    }

    if (state_ == ServiceState::kReady) {
        assert(task_);
        // 只能尝试调度OnStart
        auto msg_opt = excutor_.TrySchedule(*task_, std::move(msg));
        if (task_->coroutine.done()) {
            task_ = std::nullopt;
            state_ = ServiceState::kRunning;
            return;
        }
        if (!msg_opt) {
            return;
        }
        msg = std::move(*msg_opt);
    }
    
    if (msg->type() == MillionSessionTimeoutMsg::kType) {
        auto msg_ptr = static_cast<MillionSessionTimeoutMsg*>(msg.get());
        if (state_ == ServiceState::kReady) {
            assert(task_);
            // 超时未完成OnStart，准备销毁服务
            iservice_->OnTimeout(std::move(*task_));
            task_ = std::nullopt;
            iservice_->Send<MillionServiceStopMsg>(service_handle());
        }
        else {
            auto task_opt = excutor_.TimeoutCleanup(msg_ptr->timeout_id);
            if (task_opt) {
                iservice_->OnTimeout(std::move(*task_opt));
            }
        }
        return;
    }

    if (state_ != ServiceState::kRunning) {
        return;
    }

    auto msg_opt = excutor_.TrySchedule(std::move(msg));
    if (!msg_opt) {
        return;
    }

    auto task = iservice_->OnMsg(std::move(*msg_opt));
    excutor_.AddTask(std::move(task));
}

void Service::ProcessMsgs(size_t count) {
    for (size_t i = 0; i < count; ++i) {
        auto msg_opt = PopMsg();
        if (!msg_opt) {
            break;
        }
        ProcessMsg(std::move(*msg_opt));
    }
}

void Service::EnableSeparateWorker() {
    separate_worker_ = std::make_unique<SeparateWorker>([this] {
        SeparateThreadHandle();
    });
}

bool Service::HasSeparateWorker() const {
    return separate_worker_.operator bool();
}

void Service::SeparateThreadHandle() {
    while (true) {
        std::optional<MsgUnique> msg;
        {
            auto lock = std::unique_lock(msgs_mutex_);
            while (msgs_.empty()) {
                separate_worker_->cv.wait(lock);
            }
            msg = PopMsgWithLock();
        }
        do {
            ProcessMsg(std::move(*msg));
            if (IsExit()) {
                // 销毁服务
                service_mgr_->DeleteService(this);
                break;
            }
        } while (msg = PopMsg());
    }
}

std::optional<MsgUnique> Service::PopMsgWithLock() {
    if (msgs_.empty()) {
        return std::nullopt;
    }
    auto msg = std::move(msgs_.front());
    msgs_.pop();
    assert(msg);
    return msg;
}

} // namespace million
