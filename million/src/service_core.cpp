#include "service_core.h"

#include <cassert>

#include <million/iservice.h>

#include "million.h"
#include "service_mgr.h"
#include "session_monitor.h"

namespace million {

ServiceCore::ServiceCore(ServiceMgr* service_mgr, std::unique_ptr<IService> iservice)
    : service_mgr_(service_mgr)
    , iservice_(std::move(iservice))
    , excutor_(this) {}

ServiceCore::~ServiceCore() = default;


bool ServiceCore::PushMsg(const ServiceShared& sender, SessionId session_id, MessagePointer msg) {
    assert(msg);
    {
        auto lock = std::lock_guard(msgs_mutex_);
        if (!IsReady() && !IsStarting() && !IsRunning() && !msg.IsType<ServiceExitMsg>()) {
            return false;
        }
        auto ele = ServiceCore::MsgElement{ .sender = sender, .session_id = session_id, .msg = std::move(msg) };
        msgs_.emplace(std::move(ele));
    }
    if (HasSeparateWorker()) {
        separate_worker_->cv.notify_one();
    }
    return true;
}

std::optional<ServiceCore::MsgElement> ServiceCore::PopMsg() {
    auto lock = std::lock_guard(msgs_mutex_);
    return PopMsgWithLock();
}

bool ServiceCore::MsgQueueIsEmpty() {
    std::lock_guard guard(msgs_mutex_);
    return msgs_.empty();
}

void ServiceCore::ProcessMsg(MsgElement ele) {
    auto& sender = ele.sender;
    auto session_id = ele.session_id;
    auto& msg = ele.msg;

    if (msg.IsType<ServiceStartMsg>()) {
        if (!IsReady() && !IsStopped()) {
            // 不是可开启服务的状态
            return;
        }
        stage_ = ServiceStage::kStarting;
        if (!SessionIsSendId(session_id)) {
            service_mgr_->million().logger().LOG_ERROR("Should not receive send type messages: {}.", session_id);
            return;
        }
        auto start_msg = msg.GetMutableMessage<ServiceStartMsg>();
        if (!start_msg) {
            service_mgr_->million().logger().LOG_ERROR("Get service start msg err.");
            return;
        }
        auto task = iservice_->OnStart(ServiceHandle(sender), session_id, std::move(start_msg->with_msg));
        auto ele = excutor_.AddTask(TaskElement(std::move(sender), session_id, std::move(task)));
        if (ele) {
            // 已完成OnStart
            stage_ = ServiceStage::kRunning;
            ReplyMsg(&*ele);
        }
        return;
    }
    else if (msg.IsType<ServiceStopMsg>()) {
        if (!IsRunning()) {
            // 不是可关闭服务的状态
            return;
        }
        stage_ = ServiceStage::kStopping;
        if (!SessionIsSendId(session_id)) {
            service_mgr_->million().logger().LOG_ERROR("Should not receive send type messages: {}.", session_id);
            return;
        }
        auto stop_msg = msg.GetMutableMessage<ServiceStopMsg>();
        if (!stop_msg) {
            service_mgr_->million().logger().LOG_ERROR("Get service stop msg err.");
            return;
        }
        auto task = iservice_->OnStop(ServiceHandle(sender), session_id, std::move(stop_msg->with_msg));
        auto ele = excutor_.AddTask(TaskElement(std::move(sender), session_id, std::move(task)));
        if (ele) {
            // 已完成OnStop
            stage_ = ServiceStage::kStopped;
            ReplyMsg(&*ele);
        }
        return;
    }
    else if (msg.IsType<ServiceExitMsg>()) {
        if (!IsStopped()) {
            // 不是可退出服务的状态
            return;
        }
        stage_ = ServiceStage::kExited;
        try {
            if (!SessionIsSendId(session_id)) {
                service_mgr_->million().logger().LOG_ERROR("Should not receive send type messages: {}.", session_id);
                return;
            }
            iservice_->OnExit();
        }
        catch (const std::exception& e) {
            service_mgr_->million().logger().LOG_ERROR("Service OnExit exception occurred: {}", e.what());
        }
        catch (...) {
            service_mgr_->million().logger().LOG_ERROR("Service OnExit exception occurred: {}", "unknown exception.");
        }
        return;
    }

    // Starting/Stop 都允许处理已有协程的超时
    if (msg.IsType<SessionTimeoutMsg>()) {
        auto msg_ptr = msg.GetMessage<SessionTimeoutMsg>();
        auto&& [task, has_exception] = excutor_.TaskTimeout(msg_ptr->timeout_id);
        if (IsStarting() || IsStopping() || has_exception) {
            // 异常退出的OnStart/OnStop，默认回收当前服务
            stage_ = ServiceStage::kStopped;
            Exit();
        }
        return;
    }

    if (IsStarting()) {
        // OnStart未完成，只能尝试调度已有协程
        if (!SessionIsReplyId(session_id)) {
            return;
        }
        session_id = SessionReplyToSendId(session_id);
        auto res = excutor_.TrySchedule(session_id, std::move(msg));
        if (!std::holds_alternative<TaskElement>(res)) {
            return;
        }

        // 已完成OnStart
        stage_ = ServiceStage::kRunning;
        ReplyMsg(&std::get<TaskElement>(res));
        return;
    }

    if (IsStopping()) {
        // OnStop未完成，只能尝试调度已有协程
        if (!SessionIsReplyId(session_id)) {
            return;
        }
        session_id = SessionReplyToSendId(session_id);
        auto res = excutor_.TrySchedule(session_id, std::move(msg));
        if (!std::holds_alternative<TaskElement>(res)) {
            return;
        }

        // 已完成OnStop
        stage_ = ServiceStage::kStopped;
        ReplyMsg(&std::get<TaskElement>(res));
        return;
    }


    // 非Running阶段不开启分发消息/恢复等待中的协程
    if (!IsRunning()) {
        return;
    }

    if (SessionIsReplyId(session_id)) {
        session_id = SessionReplyToSendId(session_id);
        auto res = excutor_.TrySchedule(session_id, std::move(msg));
        if (!std::holds_alternative<TaskElement>(res)) {
            return;
        }
        // 已完成的任务
        //if (session_id == lock_task_) {
        //    lock_task_ = kSessionIdInvalid;
        //}
        ReplyMsg(&std::get<TaskElement>(res));
    }
    else if (SessionIsSendId(session_id)) {
        auto task = iservice_->OnMsg(ServiceHandle(sender), session_id, std::move(msg));
        auto ele = excutor_.AddTask(TaskElement(std::move(sender), session_id, std::move(task)));
        if (!ele) {
            return;
        }
        // 已完成的任务
        ReplyMsg(&*ele);
    }
}

void ServiceCore::ProcessMsgs(size_t count) {
    // 如果处理了Exit，则直接抛弃所有消息及未完成的任务(包括未触发超时的任务)
    for (size_t i = 0; i < count && !IsExited(); ++i) {
        auto msg_opt = PopMsg();
        if (!msg_opt) {
            break;
        }
        ProcessMsg(std::move(*msg_opt));
    }
}

bool ServiceCore::TaskExecutorIsEmpty() const {
    return excutor_.TaskQueueIsEmpty();
}

void ServiceCore::EnableSeparateWorker() {
    separate_worker_ = std::make_unique<SeparateWorker>([this] {
        SeparateThreadHandle();
    });
}

bool ServiceCore::HasSeparateWorker() const {
    return separate_worker_.operator bool();
}

void ServiceCore::SeparateThreadHandle() {
    while (true) {
        std::optional<ServiceCore::MsgElement> msg;
        {
            auto lock = std::unique_lock(msgs_mutex_);
            while (msgs_.empty()) {
                separate_worker_->cv.wait(lock);
            }
            msg = PopMsgWithLock();
        }
        do {
            ProcessMsg(std::move(*msg));
            if (IsExited()) {
                // 服务已退出，销毁
                service_mgr_->DeleteService(this);
                break;
            }
        } while (msg = PopMsg());
    }
}

std::optional<ServiceCore::MsgElement> ServiceCore::PopMsgWithLock() {
    if (msgs_.empty()) {
        return std::nullopt;
    }
    auto msg = std::move(msgs_.front());

    // 如果指定任务上锁，只能调度该任务相关的消息
    //if (lock_task_ != kSessionIdInvalid && msg.session_id == lock_task_) {
    //    return std::nullopt;
    //}

    msgs_.pop();
    assert(msg.msg);
    return msg;
}

void ServiceCore::ReplyMsg(TaskElement* ele) {
    if (ele->task.has_exception()) {
        return;
    }

    if (!ele->task.coroutine.promise().result_value) {
        service_mgr_->million().logger().LOG_ERROR("Task Session {} has no return value.", ele->session_id);
        return;
    }
    auto reply_msg = std::move(*ele->task.coroutine.promise().result_value);
    if (!reply_msg) {
        // co_return nullptr;
        return;
    }
    service_mgr()->Send(*iter_, ele->sender, SessionSendToReplyId(ele->session_id), std::move(reply_msg));
}


std::optional<SessionId> ServiceCore::Start(MessagePointer msg) {
    return iservice_->Send<ServiceStartMsg>(iservice_->service_handle(), std::move(msg));
}

std::optional<SessionId> ServiceCore::Stop(MessagePointer msg) {
    return iservice_->Send<ServiceStopMsg>(iservice_->service_handle(), std::move(msg));
}

std::optional<SessionId> ServiceCore::Exit() {
    return iservice_->Send<ServiceExitMsg>(iservice_->service_handle());
}


bool ServiceCore::IsReady() const {
    return stage_ == ServiceStage::kReady;
}

bool ServiceCore::IsStarting() const {
    return stage_ == ServiceStage::kStarting;
}

bool ServiceCore::IsRunning() const {
    return stage_ == ServiceStage::kRunning;
}

bool ServiceCore::IsStopping() const {
    return stage_ == ServiceStage::kStopping;
}

bool ServiceCore::IsStopped() const {
    return stage_ == ServiceStage::kStopped;
}

bool ServiceCore::IsExited() const {
    return stage_ == ServiceStage::kExited;
}

} // namespace million
