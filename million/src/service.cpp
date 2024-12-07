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
    if (msg->type() == MillionServiceStartMsg::type_static()) {
        on_start_task_.emplace(iservice_->OnStart());
        if (!on_start_task_->coroutine.done()) {
            // OnStart未完成，等待处理完成
            auto id = on_start_task_->coroutine.promise().session_awaiter()->waiting_session();
            auto timeout_s = on_start_task_->coroutine.promise().session_awaiter()->timeout_s();
            service_mgr()->million()->session_monitor().AddSession(service_handle(), id, timeout_s);
        }
        else {
            on_start_task_ = std::nullopt;
            state_ = ServiceState::kRunning;
        }
        return;
    }
    else if (msg->type() == MillionServiceStopMsg::type_static()) {
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
        assert(on_start_task_);
        if (msg->type() == MillionSessionTimeoutMsg::type_static()) {
            auto msg_ptr = static_cast<MillionSessionTimeoutMsg*>(msg.get());
            // 超时未完成OnStart，准备销毁服务
            on_start_task_ = std::nullopt;
            iservice_->Send<MillionServiceStopMsg>(service_handle());
            return;
        }
        // 只能尝试调度OnStart
        excutor_.TrySchedule(*on_start_task_, std::move(msg));
        if (on_start_task_->coroutine.done()) {
            on_start_task_ = std::nullopt;
            state_ = ServiceState::kRunning;
        }
        return;
    }

    if (msg->type() == MillionSessionTimeoutMsg::type_static()) {
        auto msg_ptr = static_cast<MillionSessionTimeoutMsg*>(msg.get());
        excutor_.TaskTimeout(msg_ptr->timeout_id);
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
