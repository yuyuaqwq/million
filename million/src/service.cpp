#include "service.h"

#include <cassert>

#include <million/iservice.h>

#include <protogen/ss/ss_service.pb.h>

#include "million.h"
#include "service_mgr.h"
#include "session_monitor.h"

namespace million {

Service::Service(ServiceMgr* service_mgr, std::unique_ptr<IService> iservice)
    : service_mgr_(service_mgr)
    , iservice_(std::move(iservice))
    , excutor_(this) {}

Service::~Service() = default;

void Service::Start() {
    if (state_ != ServiceState::kReady) {
        return;
    }
    iservice_->Send<ss::service::ServiceStart>(service_handle());
}

void Service::Stop() {
    if (IsStoping() || IsStop()) {
        return;
    }
    state_ == ServiceState::kStoping;
    iservice_->Send<ss::service::ServiceStop>(service_handle());
}

bool Service::IsStoping() const {
    return state_ == ServiceState::kStoping;
}

bool Service::IsStop() const {
    return state_ == ServiceState::kStop;
}

void Service::PushMsg(SessionId session_id, MsgUnique msg) {
    {
        auto lock = std::lock_guard(msgs_mutex_);
        if (IsStoping() || IsStop()) {
            // 服务退出，不再接收消息
            return;
        }
        msgs_.emplace(session_id, std::move(msg));
    }
    if (HasSeparateWorker()) {
        separate_worker_->cv.notify_one();
    }
}

std::optional<Service::MsgElement> Service::PopMsg() {
    auto lock = std::lock_guard(msgs_mutex_);
    return PopMsgWithLock();
}

bool Service::MsgQueueIsEmpty() {
    std::lock_guard guard(msgs_mutex_);
    return msgs_.empty();
}

void Service::ProcessMsg(MsgElement ele) {
    auto session_id = ele.first;
    auto& msg = ele.second;
    if (msg.IsType(ss::service::ServiceStart::GetDescriptor())) {
        auto task = iservice_->OnStart();
        if (!excutor_.AddTask(std::move(task))) {
            // 已完成
            state_ = ServiceState::kRunning;
        }
        return;
    }
    else if (msg.IsType(ss::service::ServiceStop::GetDescriptor())) {
        try {
            iservice_->OnStop();
        }
        catch (const std::exception& e) {
            service_mgr_->million().logger().Err("Service OnStop exception occurred: {}", e.what());
        }
        catch (...) {
            service_mgr_->million().logger().Err("Service OnStop exception occurred: {}", "unknown exception");
        }
        state_ = ServiceState::kStop;
        return;
    }

    if (msg.IsType(ss::service::SessionTimeout::GetDescriptor())) {
        auto msg_ptr = msg.get<ss::service::SessionTimeout>();
        auto task = excutor_.TaskTimeout(msg_ptr->timeout_id());
        if (task && state_ == ServiceState::kReady) {
            if (!task->coroutine.done() || !task->coroutine.promise().exception()) {
                return;
            }
            // 异常退出的OnStart，销毁当前服务
            Stop();
        }
        return;
    }

    if (state_ == ServiceState::kReady) {
        // OnStart未完成，只能尝试调度已有协程
        auto res = excutor_.TrySchedule(session_id, std::move(msg));
        if (!std::holds_alternative<Task<>*>(res)) {
            return;
        }
        auto task = std::get<Task<>*>(res);
        if (!task) {
            state_ = ServiceState::kRunning;
        }
        return;
    }

    if (state_ != ServiceState::kRunning) {
        return;
    }

    auto res = excutor_.TrySchedule(session_id, std::move(msg));
    if (std::holds_alternative<Task<>*>(res)) {
        return;
    }

    auto task = iservice_->OnMsg(session_id, std::move(std::get<MsgUnique>(res)));
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
        std::optional<Service::MsgElement> msg;
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

std::optional<Service::MsgElement> Service::PopMsgWithLock() {
    if (msgs_.empty()) {
        return std::nullopt;
    }
    auto msg = std::move(msgs_.front());
    msgs_.pop();
    assert(msg.second);
    return msg;
}

} // namespace million
