#pragma once

#include <cassert>

#include <memory>

#include <million/api.h>
#include <million/noncopyable.h>
#include <million/session_def.h>
#include <million/msg.h>
#include <million/service_handle.h>
#include <million/service_lock.h>
#include <million/task.h>
#include <million/logger.h>

namespace million {

class IMillion;
class MILLION_API IService : noncopyable {
public:
    IService(IMillion* imillion)
        : imillion_(imillion) {}
    virtual ~IService() = default;

public:
    std::optional<SessionId> Send(const ServiceHandle& target, MsgPtr msg);
    template <typename MsgT, typename ...Args>
    std::optional<SessionId> Send(const ServiceHandle& target, Args&&... args) {
        return Send(target, make_msg<MsgT>(std::forward<Args>(args)...));
    }

    bool SendTo(const ServiceHandle& target, SessionId session_id, MsgPtr msg);
    template <typename MsgT, typename ...Args>
    bool SendTo(const ServiceHandle& target, SessionId session_id, Args&&... args) {
        return SendTo(target, session_id, make_msg<MsgT>(std::forward<Args>(args)...));
    }

    bool Reply(const ServiceHandle& target, SessionId session_id, MsgPtr msg);
    template <typename MsgT, typename ...Args>
    bool Reply(const ServiceHandle& target, SessionId session_id, Args&&... args) {
        return Reply(target, session_id, make_msg<MsgT>(std::forward<Args>(args)...));
    }

    template <typename MsgT>
    SessionAwaiter<MsgT> Recv(SessionId session_id) {
        // 0表示默认超时时间
        return SessionAwaiter<MsgT>(session_id, 0, false);
    }

    template <typename MsgT>
    SessionAwaiter<MsgT> RecvWithTimeout(SessionId session_id, uint32_t timeout_s) {
        return SessionAwaiter<MsgT>(session_id, timeout_s, false);
    }

    template <typename MsgT>
    SessionAwaiter<MsgT> RecvOrNull(SessionId session_id) {
        // 0表示默认超时时间
        return SessionAwaiter<MsgT>(session_id, 0, true);
    }

    template <typename MsgT>
    SessionAwaiter<MsgT> RecvOrNullWithTimeout(SessionId session_id, uint32_t timeout_s) {
        return SessionAwaiter<MsgT>(session_id, timeout_s, true);
    }


    template <typename RecvMsgT, typename SendMsgT, typename ...Args>
    SessionAwaiter<RecvMsgT> Call(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send<SendMsgT>(target, std::forward<Args>(args)...);
        return Recv<RecvMsgT>(session_id.value());
    }
    template <typename MsgT, typename ...Args>
    SessionAwaiter<MsgT> Call(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send<MsgT>(target, std::forward<Args>(args)...);
        return Recv<MsgT>(session_id.value());
    }

    template <typename RecvMsgT, typename SendMsgT, typename ...Args>
    SessionAwaiter<RecvMsgT> CallWithTimeout(const ServiceHandle& target, uint32_t timeout_s, Args&&... args) {
        auto session_id = Send<SendMsgT>(target, std::forward<Args>(args)...);
        return RecvWithTimeout<RecvMsgT>(session_id.value(), timeout_s);
    }
    template <typename MsgT, typename ...Args>
    SessionAwaiter<MsgT> CallWithTimeout(const ServiceHandle& target, uint32_t timeout_s, Args&&... args) {
        auto session_id = Send<MsgT>(target, std::forward<Args>(args)...);
        return RecvWithTimeout<MsgT>(session_id.value(), timeout_s);
    }

    template <typename RecvMsgT, typename SendMsgT, typename ...Args>
    SessionAwaiter<RecvMsgT> CallOrNull(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send<SendMsgT>(target, std::forward<Args>(args)...);
        return RecvOrNull<RecvMsgT>(session_id.value());
    }
    template <typename MsgT, typename ...Args>
    SessionAwaiter<MsgT> CallOrNull(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send<MsgT>(target, std::forward<Args>(args)...);
        return RecvOrNull<MsgT>(session_id.value());
    }

    template <typename RecvMsgT, typename SendMsgT, typename ...Args>
    SessionAwaiter<RecvMsgT> CallOrNullWithTimeout(const ServiceHandle& target, uint32_t timeout_s, Args&&... args) {
        auto session_id = Send<SendMsgT>(target, std::forward<Args>(args)...);
        return RecvOrNullWithTimeout<RecvMsgT>(session_id.value(), timeout_s);
    }
    template <typename MsgT, typename ...Args>
    SessionAwaiter<MsgT> CallOrNullWithTimeout(const ServiceHandle& target, uint32_t timeout_s, Args&&... args) {
        auto session_id = Send<MsgT>(target, std::forward<Args>(args)...);
        return RecvOrNullWithTimeout<MsgT>(session_id.value(), timeout_s);
    }

    void Timeout(uint32_t tick, MsgPtr msg);
    template <typename MsgT, typename ...Args>
    void Timeout(uint32_t tick, Args&&... args) {
        Timeout(tick, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

public:
    Logger& logger();
    IMillion& imillion() { return *imillion_; }
    const ServiceHandle& service_handle() const { return service_handle_; }

protected:
    virtual bool OnInit() { return true; }
    virtual Task<MsgPtr> OnStart(ServiceHandle sender, SessionId session_id, MsgPtr with_msg) { co_return nullptr; }
    virtual Task<MsgPtr> OnMsg(ServiceHandle sender, SessionId session_id, MsgPtr msg) { co_return nullptr; }
    virtual Task<MsgPtr> OnStop(ServiceHandle sender, SessionId session_id, MsgPtr with_msg) { co_return nullptr; }
    virtual void OnExit() { }

private:
    void set_service_handle(const ServiceHandle& handle) { service_handle_ = handle; }

private:
    friend class ServiceCore;
    friend class ServiceMgr;

    IMillion* imillion_;
    ServiceHandle service_handle_;
};

#define MILLION_MSG_DISPATCH(MILLION_SERVICE_TYPE_) \
    using _MILLION_SERVICE_TYPE_ = MILLION_SERVICE_TYPE_; \
    virtual ::million::Task<::million::MsgPtr> OnMsg(::million::ServiceHandle sender, ::million::SessionId session_id, ::million::MsgPtr msg) override { \
        auto iter = _MILLION_MSG_HANDLE_MAP_.find(msg.GetTypeKey()); \
        if (iter != _MILLION_MSG_HANDLE_MAP_.end()) { \
             co_return co_await (this->*iter->second)(sender, session_id, std::move(msg)); \
        } \
        co_return nullptr; \
    } \
    ::std::unordered_map<::million::MsgTypeKey, ::million::Task<::million::MsgPtr>(_MILLION_SERVICE_TYPE_::*)(const ::million::ServiceHandle&, ::million::SessionId, ::million::MsgPtr)> _MILLION_MSG_HANDLE_MAP_ \

#define MILLION_MSG_HANDLE_TEMPLATE(MSG_TYPE_, MSG_PTR_NAME_, GET_MSG_METHOD_, CONST_) \
    ::million::Task<::million::MsgPtr> _MILLION_MSG_HANDLE_##MSG_TYPE_##_I(const ::million::ServiceHandle& sender, ::million::SessionId session_id, ::million::MsgPtr msg_) { \
        auto msg = msg_.GET_MSG_METHOD_<MSG_TYPE_>(); \
        return _MILLION_MSG_HANDLE_##MSG_TYPE_##_II(sender, session_id, std::move(msg_), msg); \
    } \
    const bool _MILLION_MSG_HANDLE_REGISTER_##MSG_TYPE_ =  \
        [this] { \
            auto res = _MILLION_MSG_HANDLE_MAP_.emplace(::million::GetMsgTypeKey<MSG_TYPE_>(), \
                &_MILLION_SERVICE_TYPE_::_MILLION_MSG_HANDLE_##MSG_TYPE_##_I \
            ); \
            assert(res.second); \
            return true; \
        }(); \
    ::million::Task<::million::MsgPtr> _MILLION_MSG_HANDLE_##MSG_TYPE_##_II(const ::million::ServiceHandle& sender, ::million::SessionId session_id, ::million::MsgPtr MSG_PTR_NAME_##_, CONST_ MSG_TYPE_* MSG_PTR_NAME_)

#define MILLION_MSG_HANDLE(MSG_TYPE_, MSG_PTR_NAME_) \
    MILLION_MSG_HANDLE_TEMPLATE(MSG_TYPE_, MSG_PTR_NAME_, GetMsg, const)

#define MILLION_MUT_MSG_HANDLE(MSG_TYPE_, MSG_PTR_NAME_) \
    MILLION_MSG_HANDLE_TEMPLATE(MSG_TYPE_, MSG_PTR_NAME_, GetMutMsg)


// 持久会话循环参考
// 使用持久会话有些需要注意的地方：
// 一个服务可以创建多个持久会话，创建后相当于对外提供一个持续性的，针对某个SessionId的服务端(服务协程)
// 同一个对端服务，同一时间只能存在一个连接到此持久会话的客户端(服务协程)

// 需要创建新的协程，不能直接co_await OnMsg(会导致当前持久会话协程被阻塞，无法处理新的持久会话消息)
// SendTo如果考虑性能可以直接替换成当前服务的Service::ProcessMsg，但是是私有类
#define MILLION_PERSISTENT_SESSION_MSG_LOOP(START_MSG_TYPE_, STOP_MSG_TYPE_) \
    MILLION_MSG_HANDLE(START_MSG_TYPE_, msg) { \
        do { \
            auto recv_msg = co_await ::million::SessionAwaiterBase(session_id, ::million::kSessionNeverTimeout, false); \
            auto msg_type_key = recv_msg.GetTypeKey(); \
            imillion().SendTo(sender, service_handle(), session_id, std::move(recv_msg)); \
            if (::million::GetMsgTypeKey<STOP_MSG_TYPE_>()) { \
                break; \
            } \
        } while (true);\
        co_return; \
    }

} // namespace million