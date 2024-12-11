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
            // �����˳������ٽ�����Ϣ
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
        auto task = iservice_->OnStart();
        if (!excutor_.AddTask(std::move(task))) {
            // �����
            state_ = ServiceState::kRunning;
        }
        return;
    }
    else if (msg->type() == MillionServiceStopMsg::type_static()) {
        // if (state_ == ServiceState::kReady) {
            // OnStartδ��ɣ�ֱ���˳�
        iservice_->OnExit();
        state_ = ServiceState::kExit;
        return;
        // }
        // �ȴ�OnStop���
        return;
    }

    if (msg->type() == MillionSessionTimeoutMsg::type_static()) {
        auto msg_ptr = static_cast<MillionSessionTimeoutMsg*>(msg.get());
        auto task = excutor_.TaskTimeout(msg_ptr->timeout_id);
        if (task && state_ == ServiceState::kReady) {
            if (!task->coroutine.done() || !task->coroutine.promise().exception()) {
                return;
            }
            // �쳣�˳���OnStart�����ٵ�ǰ����
            iservice_->Send<MillionServiceStopMsg>(service_handle());
        }
        return;
    }

    if (state_ == ServiceState::kReady) {
        // OnStartδ��ɣ�ֻ�ܳ��Ե�������Э��
        auto res = excutor_.TrySchedule(std::move(msg));
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

    auto res = excutor_.TrySchedule(std::move(msg));
    if (std::holds_alternative<Task<>*>(res)) {
        return;
    }

    auto task = iservice_->OnMsg(std::move(std::get<MsgUnique>(res)));
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
                // ���ٷ���
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
