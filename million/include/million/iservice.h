#pragma once

#include <cassert>

#include <memory>
#include <typeindex>

#include <million/api.h>
#include <million/noncopyable.h>
#include <million/session_def.h>
#include <million/msg.h>
#include <million/service_handle.h>
#include <million/service_lock.h>
#include <million/task.h>
#include <million/logger.h>

namespace million {

using ServiceTypeKey = std::type_index;

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
    virtual Task<MsgPtr> OnMsg(ServiceHandle sender, SessionId session_id, MsgPtr msg) { co_return co_await MsgDispatch(std::move(sender), session_id, std::move(msg)); }
    virtual Task<MsgPtr> OnStop(ServiceHandle sender, SessionId session_id, MsgPtr with_msg) { co_return nullptr; }
    virtual void OnExit() { }


    virtual ServiceTypeKey GetTypeKey() = 0;

    Task<MsgPtr> MsgDispatch(ServiceHandle sender, SessionId session_id, MsgPtr msg) {
        auto service_type_key = GetTypeKey();
        auto msg_type_key = msg.GetTypeKey();
        auto it = msg_handlers_.find({ service_type_key, msg_type_key });
        if (it != msg_handlers_.end()) {
            co_return co_await it->second(this, std::move(sender), session_id, std::move(msg));
        }
        co_return nullptr;
    }

    template <typename MsgT, typename ServiceT>
    static void RegisterMsgHandler(Task<MsgPtr>(ServiceT::* handler)(const ServiceHandle& sender, SessionId session_id, MsgPtr msg_ptr, MsgT* msg)) {
        msg_handlers_[{ typeid(ServiceT), GetMsgTypeKey<MsgT>() }] = [handler](IService* iservice, ServiceHandle sender, SessionId session_id, MsgPtr msg_ptr) -> Task<MsgPtr> {
            ServiceT* service = static_cast<ServiceT*>(iservice);
            if constexpr (std::is_const_v<std::remove_pointer_t<MsgT>>) {
                auto* msg = msg_ptr.GetMsg<MsgT>();
                co_return co_await (service->*handler)(std::move(sender), session_id, std::move(msg_ptr), msg);
            }
            else {
                auto* msg = msg_ptr.GetMutMsg<MsgT>();
                co_return co_await (service->*handler)(std::move(sender), session_id, std::move(msg_ptr), msg);
            }
        };
    }

    template <typename MsgT, typename ServiceT>
    static bool StaticRegisterMsgHandler() {
        Task<MsgPtr>(ServiceT::*handler)(const ServiceHandle&, SessionId, MsgPtr, MsgT*) = &ServiceT::OnHandle;
        static bool registered = [&] {
            RegisterMsgHandler<MsgT>(handler);
            return true;
        }();
        return registered;
    }

private:
    void set_service_handle(const ServiceHandle& handle) { service_handle_ = handle; }

private:
    friend class ServiceCore;
    friend class ServiceMgr;

    IMillion* imillion_;
    ServiceHandle service_handle_;

    struct TypeIndexPair {
        ServiceTypeKey service_type_key;
        MsgTypeKey msg_type_key;

        bool operator==(const TypeIndexPair& other) const {
            return service_type_key == other.service_type_key && msg_type_key == other.msg_type_key;
        }
    };

    struct TypeIndexPairHash {
        size_t operator()(const TypeIndexPair& p) const {
            return p.service_type_key.hash_code() ^ p.msg_type_key;
        }
    };

    static inline std::unordered_map<TypeIndexPair
        , std::function<Task<MsgPtr>(IService*, ServiceHandle, SessionId, MsgPtr)>
        , TypeIndexPairHash> msg_handlers_;
};

//// 持久会话循环参考
//// 使用持久会话有些需要注意的地方：
//// 一个服务可以创建多个持久会话，创建后相当于对外提供一个持续性的，针对某个SessionId的服务端(服务协程)
//// 同一个对端服务，同一时间只能存在一个连接到此持久会话的客户端(服务协程)
//
//// 需要创建新的协程，不能直接co_await OnMsg(会导致当前持久会话协程被阻塞，无法处理新的持久会话消息)
//// SendTo如果考虑性能可以直接替换成当前服务的Service::ProcessMsg，但是是私有类
//#define MILLION_PERSISTENT_SESSION_MSG_LOOP(START_MSG_TYPE_, STOP_MSG_TYPE_) \
//    MILLION_MSG_HANDLE(START_MSG_TYPE_, msg) { \
//        do { \
//            auto recv_msg = co_await ::million::SessionAwaiterBase(session_id, ::million::kSessionNeverTimeout, false); \
//            auto msg_type_key = recv_msg.GetTypeKey(); \
//            imillion().SendTo(sender, service_handle(), session_id, std::move(recv_msg)); \
//            if (::million::GetMsgTypeKey<STOP_MSG_TYPE_>()) { \
//                break; \
//            } \
//        } while (true);\
//        co_return; \
//    }


#define MILLION_SERVICE_DEFINE(SERVICE_CLASS) \
    private:\
        using SELF_CLASS_ = SERVICE_CLASS; \
    virtual ::million::ServiceTypeKey GetTypeKey() override { return typeid(SELF_CLASS_); }
    

#define CONCAT(a, b) a##b          // 直接拼接
#define CONCAT_LINE(name, line) CONCAT(name, line)  // 先展开 line，再拼接

#define MILLION_MSG_HANDLE(MSG_TYPE, MSG_NAME) \
    static inline auto CONCAT_LINE(on_handle_, __LINE__)##_  = StaticRegisterMsgHandler<MSG_TYPE, SELF_CLASS_>(); \
    ::million::Task<::million::MsgPtr> OnHandle(const ::million::ServiceHandle& sender, ::million::SessionId session_id, ::million::MsgPtr msg_, MSG_TYPE* MSG_NAME)

} // namespace million