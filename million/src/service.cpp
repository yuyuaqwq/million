#include "service.h"

#include <cassert>

#include <million/iservice.h>

#include "service_mgr.h"
#include "million_msg.h"

namespace million {

Service::Service(ServiceMgr* service_mgr, std::unique_ptr<IService> iservice)
    : service_mgr_(service_mgr)
    , iservice_(std::move(iservice))
    , excutor_(this) {}

Service::~Service() = default;

bool Service::IsStop() const {
    return state_ == ServiceState::kStop;
}

void Service::PushMsg(MsgUnique msg) {
    {
        auto lock = std::lock_guard(msgs_mutex_);
        if (state_ == ServiceState::kStop) {
            // 服务关闭，不再接收消息
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
        auto task = iservice_->OnStart();
        excutor_.AddTask(std::move(task));
        state_ = ServiceState::kRunning;
        return;
    }
    else if (msg->type() == MillionServiceExitMsg::kType) {
        iservice_->OnExit();
        state_ = ServiceState::kStop;
        return;
    }
    else if (msg->type() == MillionSessionTimeoutMsg::kType) {
        auto msg_ptr = static_cast<MillionSessionTimeoutMsg*>(msg.get());
        excutor_.TimeoutCleanup(msg_ptr->timeout_id);
        return;
    }

    auto session_id = msg->session_id();
    auto msg_opt = excutor_.TrySchedule(session_id, std::move(msg));
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
            if (IsStop()) {
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
