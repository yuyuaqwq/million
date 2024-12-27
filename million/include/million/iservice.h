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

protected:
    SessionId NewSession();

    SessionId Send(const ServiceHandle& target, MsgUnique msg);
    template <typename MsgT, typename ...Args>
    SessionId Send(const ServiceHandle& target, Args&&... args) {
        return Send(target, make_msg<MsgT>(std::forward<Args>(args)...));
    }

    bool SendTo(const ServiceHandle& target, SessionId session_id, MsgUnique msg);
    template <typename MsgT, typename ...Args>
    bool SendTo(const ServiceHandle& target, SessionId session_id, Args&&... args) {
        return SendTo(target, session_id, make_msg<MsgT>(std::forward<Args>(args)...));
    }

    bool Reply(const ServiceHandle& target, SessionId session_id, MsgUnique msg);
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
        return Recv<RecvMsgT>(session_id);
    }
    template <typename MsgT, typename ...Args>
    SessionAwaiter<MsgT> Call(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send<MsgT>(target, std::forward<Args>(args)...);
        return Recv<MsgT>(session_id);
    }

    template <typename RecvMsgT, typename SendMsgT, typename ...Args>
    SessionAwaiter<RecvMsgT> CallWithTimeout(const ServiceHandle& target, uint32_t timeout_s, Args&&... args) {
        auto session_id = Send<SendMsgT>(target, std::forward<Args>(args)...);
        return RecvWithTimeout<RecvMsgT>(session_id, timeout_s);
    }
    template <typename MsgT, typename ...Args>
    SessionAwaiter<MsgT> CallWithTimeout(const ServiceHandle& target, uint32_t timeout_s, Args&&... args) {
        auto session_id = Send<MsgT>(target, std::forward<Args>(args)...);
        return RecvWithTimeout<MsgT>(session_id, timeout_s);
    }

    template <typename RecvMsgT, typename SendMsgT, typename ...Args>
    SessionAwaiter<RecvMsgT> CallOrNull(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send<SendMsgT>(target, std::forward<Args>(args)...);
        return RecvOrNull<RecvMsgT>(session_id);
    }
    template <typename MsgT, typename ...Args>
    SessionAwaiter<MsgT> CallOrNull(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send<MsgT>(target, std::forward<Args>(args)...);
        return RecvOrNull<MsgT>(session_id);
    }

    template <typename RecvMsgT, typename SendMsgT, typename ...Args>
    SessionAwaiter<RecvMsgT> CallOrNullWithTimeout(const ServiceHandle& target, uint32_t timeout_s, Args&&... args) {
        auto session_id = Send<SendMsgT>(target, std::forward<Args>(args)...);
        return RecvOrNullWithTimeout<RecvMsgT>(session_id, timeout_s);
    }
    template <typename MsgT, typename ...Args>
    SessionAwaiter<MsgT> CallOrNullWithTimeout(const ServiceHandle& target, uint32_t timeout_s, Args&&... args) {
        auto session_id = Send<MsgT>(target, std::forward<Args>(args)...);
        return RecvOrNullWithTimeout<MsgT>(session_id, timeout_s);
    }


    void Timeout(uint32_t tick, MsgUnique msg);
    template <typename MsgT, typename ...Args>
    void Timeout(uint32_t tick, Args&&... args) {
        Timeout(tick, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

    void EnableSeparateWorker();

    virtual bool OnInit(MsgUnique init_msg) { return true; }
    virtual Task<> OnStart(ServiceHandle sender, SessionId session_id) { co_return; }
    virtual Task<> OnMsg(ServiceHandle sender, SessionId session_id, MsgUnique msg) { co_return; }
    virtual void OnStop(ServiceHandle sender, SessionId session_id) { }
    virtual void OnExit(ServiceHandle sender, SessionId session_id) { }

    Logger& logger();

private:
    void set_service_handle(const ServiceHandle& handle) { service_handle_ = handle; }

public:
    IMillion& imillion() { return *imillion_; }
    const ServiceHandle& service_handle() const { return service_handle_; }
    
private:
    friend class Service;
    friend class ServiceMgr;

    IMillion* imillion_;
    ServiceHandle service_handle_;
};

#define MILLION_MSG_DISPATCH(MILLION_SERVICE_TYPE_) \
    using _MILLION_SERVICE_TYPE_ = MILLION_SERVICE_TYPE_; \
    virtual ::million::Task<> OnMsg(::million::ServiceHandle sender, ::million::SessionId session_id, ::million::MsgUnique msg) override { \
        auto iter = _MILLION_MSG_HANDLE_MAP_.find(msg.GetTypeKey()); \
        if (iter != _MILLION_MSG_HANDLE_MAP_.end()) { \
            co_await (this->*iter->second)(sender, session_id, std::move(msg)); \
        } \
        co_return; \
    } \
    ::std::unordered_map<::million::MsgTypeKey, ::million::Task<>(_MILLION_SERVICE_TYPE_::*)(const ::million::ServiceHandle& sender, ::million::SessionId session_id, ::million::MsgUnique)> _MILLION_MSG_HANDLE_MAP_ \


#define MILLION_PROTO_MSG_HANDLE(MSG_TYPE_, MSG_PTR_NAME_) \
    ::million::Task<> _MILLION_MSG_HANDLE_##MSG_TYPE_##_I(const ::million::ServiceHandle& sender, ::million::SessionId session_id, ::million::MsgUnique MILLION_MSG_) { \
        auto msg = ::std::unique_ptr<MSG_TYPE_>(static_cast<MSG_TYPE_*>(MILLION_MSG_.release())); \
        co_await _MILLION_MSG_HANDLE_##MSG_TYPE_##_II(sender, session_id, std::move(msg)); \
        co_return; \
    } \
    const bool _MILLION_MSG_HANDLE_REGISTER_##MSG_TYPE_ =  \
        [this] { \
            auto res = _MILLION_MSG_HANDLE_MAP_.emplace(reinterpret_cast<::million::MsgTypeKey>(MSG_TYPE_::GetDescriptor()), \
                &_MILLION_SERVICE_TYPE_::_MILLION_MSG_HANDLE_##MSG_TYPE_##_I \
            ); \
            assert(res.second); \
            return true; \
        }(); \
    ::million::Task<> _MILLION_MSG_HANDLE_##MSG_TYPE_##_II(const ::million::ServiceHandle& sender, ::million::SessionId session_id, ::std::unique_ptr<MSG_TYPE_> MSG_PTR_NAME_)

#define MILLION_CPP_MSG_HANDLE(MSG_TYPE_, MSG_PTR_NAME_) \
    ::million::Task<> _MILLION_MSG_HANDLE_##MSG_TYPE_##_I(const ::million::ServiceHandle& sender, ::million::SessionId session_id, ::million::MsgUnique MILLION_MSG_) { \
        auto msg = ::std::unique_ptr<MSG_TYPE_>(static_cast<MSG_TYPE_*>(MILLION_MSG_.release())); \
        co_await _MILLION_MSG_HANDLE_##MSG_TYPE_##_II(sender, session_id, std::move(msg)); \
        co_return; \
    } \
    const bool _MILLION_MSG_HANDLE_REGISTER_##MSG_TYPE_ =  \
        [this] { \
            auto res = _MILLION_MSG_HANDLE_MAP_.emplace(reinterpret_cast<::million::MsgTypeKey>(&MSG_TYPE_::type_static()), \
                &_MILLION_SERVICE_TYPE_::_MILLION_MSG_HANDLE_##MSG_TYPE_##_I \
            ); \
            assert(res.second); \
            return true; \
        }(); \
    ::million::Task<> _MILLION_MSG_HANDLE_##MSG_TYPE_##_II(const ::million::ServiceHandle& sender, ::million::SessionId session_id, ::std::unique_ptr<MSG_TYPE_> MSG_PTR_NAME_)

// 使用长会话有些需要注意的地方：
// 一个服务可以创建多个长会话，创建后相当于对外提供一个持续性的，针对某个SessionId的服务端(服务协程)
// 同一个对端服务，同一时间只能存在一个连接到此长会话的客户端(服务协程)

// 需要创建新的协程，不能直接co_await OnMsg(会导致当前长会话协程被阻塞，无法处理新的长会话消息)
// SendTo如果考虑性能可以直接替换成当前服务的Service::ProcessMsg，但是是私有类
#define MILLION_PERSISTENT_SESSION_MSG_LOOP(START_TYPE_, START_MSG_TYPE_, STOP_MSG_TYPE_KEY_) \
    MILLION_##START_TYPE_##_MSG_HANDLE(START_MSG_TYPE_, msg) { \
        do { \
            auto recv_msg = co_await ::million::SessionAwaiterBase(session_id, ::million::kSessionNeverTimeout, false); \
            imillion().SendTo(sender, service_handle(), session_id, std::move(recv_msg)); \
            if (recv_msg.GetTypeKey() == reinterpret_cast<::million::MsgTypeKey>(STOP_MSG_TYPE_KEY_)) { \
                break; \
            } \
        } while (true);\
        co_return; \
    }

} // namespace million