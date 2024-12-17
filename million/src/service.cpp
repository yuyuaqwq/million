#include "service.h"

#include <cassert>

#include <million/iservice.h>

#include "million.h"
#include "service_mgr.h"
#include "session_monitor.h"

namespace million {


Service::Service(ServiceMgr* service_mgr, std::unique_ptr<IService> iservice)
    : service_mgr_(service_mgr)
    , iservice_(std::move(iservice))
    , excutor_(this) {}

Service::~Service() = default;



void Service::PushMsg(const ServiceHandle& sender, SessionId session_id, MsgUnique msg) {
    {
        auto lock = std::lock_guard(msgs_mutex_);
        if (!IsReady() && !IsStarting() && !IsRunning() && !msg.IsType(ServiceExitMsg::type_static())) {
            return;
        }
        Service::MsgElement ele{ .sender = sender, .session_id = session_id, .msg = std::move(msg) };
        msgs_.emplace(std::move(ele));
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
    auto& sender = ele.sender;
    auto session_id = ele.session_id;
    auto& msg = ele.msg;
    if (msg.IsType(ServiceStartMsg::type_static())) {
        if (!IsReady() && !IsStop()) {
            // ���ǿɿ��������״̬
            return;
        }
        state_ = kStarting;
        auto task = iservice_->OnStart(sender, session_id);
        if (!excutor_.AddTask(std::move(task))) {
            // �����OnStart
            state_ = kRunning;
        }
        return;
    }
    else if (msg.IsType(ServiceStopMsg::type_static())) {
        if (!IsRunning()) {
            // ���ǿɹرշ����״̬
            return;
        }
        state_ = kStop;
        try {
            iservice_->OnStop(sender, session_id);
        }
        catch (const std::exception& e) {
            service_mgr_->million().logger().Err("Service OnStop exception occurred: {}", e.what());
        }
        catch (...) {
            service_mgr_->million().logger().Err("Service OnStop exception occurred: {}", "unknown exception");
        }
        return;
    }
    else if (msg.IsType(ServiceExitMsg::type_static())) {
        if (!IsStop()) {
            // ���ǿ��˳������״̬
            return;
        }
        state_ = kExit;
        try {
            iservice_->OnExit(sender, session_id);
        }
        catch (const std::exception& e) {
            service_mgr_->million().logger().Err("Service OnExit exception occurred: {}", e.what());
        }
        catch (...) {
            service_mgr_->million().logger().Err("Service OnExit exception occurred: {}", "unknown exception");
        }
        return;
    }

    // Starting/Stop ������������Э�̵ĳ�ʱ
    if (msg.IsType(SessionTimeoutMsg::type_static())) {
        auto msg_ptr = msg.get<SessionTimeoutMsg>();
        auto task = excutor_.TaskTimeout(msg_ptr->timeout_id);
        if (task && IsStarting()) {
            if (!task->coroutine.done() || !task->coroutine.promise().exception()) {
                return;
            }
            // �쳣�˳���OnStart��Ĭ�ϻ��յ�ǰ����
            state_ = kStop;
            Exit();
        }
        return;
    }

    if (IsReady()) {
        // OnStartδ��ɣ�ֻ�ܳ��Ե�������Э��
        auto res = excutor_.TrySchedule(session_id, std::move(msg));
        if (!std::holds_alternative<Task<>*>(res)) {
            return;
        }
        auto task = std::get<Task<>*>(res);
        if (!task) {
            // �����OnStart
            state_ = kRunning;
        }
        return;
    }

    // ��Running�׶β������ַ���Ϣ/�ָ��ȴ��е�Э��
    if (!IsRunning()) {
        return;
    }

    auto res = excutor_.TrySchedule(session_id, std::move(msg));
    if (std::holds_alternative<Task<>*>(res)) {
        return;
    }

    auto task = iservice_->OnMsg(sender, session_id, std::move(std::get<MsgUnique>(res)));
    excutor_.AddTask(std::move(task));
}

void Service::ProcessMsgs(size_t count) {
    // ���������Exit����ֱ������������Ϣ��δ��ɵ�����(����δ������ʱ������)
    for (size_t i = 0; i < count && !IsExit(); ++i) {
        auto msg_opt = PopMsg();
        if (!msg_opt) {
            break;
        }
        ProcessMsg(std::move(*msg_opt));
    }
}

bool Service::TaskExecutorIsEmpty() const {
    return excutor_.TaskQueueIsEmpty();
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
            if (IsExit()) {
                // �������˳�������
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
    assert(msg.msg);
    return msg;
}


SessionId Service::Start() {
    return iservice_->Send<ServiceStartMsg>(service_handle());
}

SessionId Service::Stop() {
    return iservice_->Send<ServiceStopMsg>(service_handle());
}

SessionId Service::Exit() {
    return iservice_->Send<ServiceExitMsg>(service_handle());
}


bool Service::IsReady() const {
    return state_ == kReady;
}

bool Service::IsStarting() const {
    return state_ == kStarting;
}

bool Service::IsRunning() const {
    return state_ == kRunning;
}

bool Service::IsStop() const {
    return state_ == kStop;
}

bool Service::IsExit() const {
    return state_ == kExit;
}


} // namespace million
