#pragma once

#include <cassert>

#include <memory>
#include <typeindex>

#include <million/api.h>
#include <million/noncopyable.h>
#include <million/session_def.h>
#include <million/message.h>
#include <million/service_handle.h>
#include <million/service_lock.h>
#include <million/task.h>
#include <million/logger.h>

namespace million {

using ServiceTypeKey = std::type_index;

template <typename MessageT, typename ServiceT>
class AutoRegisterMsgHandler {
public:
    using OnHandlePtr = Task<MessagePointer>(ServiceT::*)(const ServiceHandle&, SessionId, MessagePointer, MessageT*);

    AutoRegisterMsgHandler() {
        handler_ = &ServiceT::OnHandle;
        ServiceT:: BindMsgHandler(handler_);
    }

    ~AutoRegisterMsgHandler() {
        ServiceT:: RemoveMsgHandler(handler_);
    }

private:
    OnHandlePtr handler_;
};

class IMillion;
class MILLION_API IService : public noncopyable {
public:
    IService(IMillion* imillion)
        : imillion_(imillion) {}
    virtual ~IService() = default;

public:
    std::optional<SessionId> Send(const ServiceHandle& target, MessagePointer msg);

    template <typename MsgT, typename ...Args>
    std::optional<SessionId> Send(const ServiceHandle& target, Args&&... args) {
        return Send(target, make_message<MsgT>(std::forward<Args>(args)...));
    }

    bool SendTo(const ServiceHandle& target, SessionId session_id, MessagePointer msg);

    template <typename MsgT, typename ...Args>
    bool SendTo(const ServiceHandle& target, SessionId session_id, Args&&... args) {
        return SendTo(target, session_id, make_message<MsgT>(std::forward<Args>(args)...));
    }

    bool Reply(const ServiceHandle& target, SessionId session_id, MessagePointer msg);

    template <typename MsgT, typename ...Args>
    bool Reply(const ServiceHandle& target, SessionId session_id, Args&&... args) {
        return Reply(target, session_id, make_message<MsgT>(std::forward<Args>(args)...));
    }

    SessionAwaiterBase Recv(SessionId session_id) {
        return SessionAwaiterBase(session_id, 0, false);
    }

    template <typename MsgT>
    SessionAwaiter<MsgT> Recv(SessionId session_id) {
        // 0表示默认超时时间
        return SessionAwaiter<MsgT>(session_id, 0, false);
    }

    SessionAwaiterBase RecvWithTimeout(SessionId session_id, uint32_t timeout_s) {
        return SessionAwaiterBase(session_id, timeout_s, false);
    }

    template <typename MsgT>
    SessionAwaiter<MsgT> RecvWithTimeout(SessionId session_id, uint32_t timeout_s) {
        return SessionAwaiter<MsgT>(session_id, timeout_s, false);
    }

    SessionAwaiterBase RecvOrNull(SessionId session_id) {
        return SessionAwaiterBase(session_id, 0, true);
    }

    template <typename MsgT>
    SessionAwaiter<MsgT> RecvOrNull(SessionId session_id) {
        // 0表示默认超时时间
        return SessionAwaiter<MsgT>(session_id, 0, true);
    }

    SessionAwaiterBase RecvOrNullWithTimeout(SessionId session_id, uint32_t timeout_s) {
        return SessionAwaiterBase(session_id, timeout_s, true);
    }

    template <typename MsgT>
    SessionAwaiter<MsgT> RecvOrNullWithTimeout(SessionId session_id, uint32_t timeout_s) {
        return SessionAwaiter<MsgT>(session_id, timeout_s, true);
    }


    template <typename SendMsgT, typename RecvMsgT, typename ...SendArgsT>
    SessionAwaiter<RecvMsgT> Call(const ServiceHandle& target, SendArgsT&&... args) {
        auto session_id = Send<SendMsgT>(target, std::forward<SendArgsT>(args)...);
        return Recv<RecvMsgT>(session_id.value());
    }

    template <typename MsgT, typename ...Args>
    SessionAwaiterBase Call(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send<MsgT>(target, std::forward<Args>(args)...);
        return Recv(session_id.value());
    }

    template <typename SendMsgT, typename RecvMsgT, typename ...SendArgsT>
    SessionAwaiter<RecvMsgT> CallWithTimeout(const ServiceHandle& target, uint32_t timeout_s, SendArgsT&&... args) {
        auto session_id = Send<SendMsgT>(target, std::forward<SendArgsT>(args)...);
        return RecvWithTimeout<RecvMsgT>(session_id.value(), timeout_s);
    }

    template <typename MsgT, typename ...Args>
    SessionAwaiterBase CallWithTimeout(const ServiceHandle& target, uint32_t timeout_s, Args&&... args) {
        auto session_id = Send<MsgT>(target, std::forward<Args>(args)...);
        return RecvWithTimeout(session_id.value(), timeout_s);
    }

    template <typename SendMsgT, typename RecvMsgT, typename ...SendArgsT>
    SessionAwaiter<RecvMsgT> CallOrNull(const ServiceHandle& target, SendArgsT&&... args) {
        auto session_id = Send<SendMsgT>(target, std::forward<SendArgsT>(args)...);
        return RecvOrNull<RecvMsgT>(session_id.value());
    }

    template <typename MsgT, typename ...Args>
    SessionAwaiterBase CallOrNull(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send<MsgT>(target, std::forward<Args>(args)...);
        return RecvOrNull(session_id.value());
    }

    template <typename SendMsgT, typename RecvMsgT, typename ...SendArgsT>
    SessionAwaiter<RecvMsgT> CallOrNullWithTimeout(const ServiceHandle& target, uint32_t timeout_s, SendArgsT&&... args) {
        auto session_id = Send<SendMsgT>(target, std::forward<SendArgsT>(args)...);
        return RecvOrNullWithTimeout<RecvMsgT>(session_id.value(), timeout_s);
    }

    template <typename MsgT, typename ...Args>
    SessionAwaiterBase CallOrNullWithTimeout(const ServiceHandle& target, uint32_t timeout_s, Args&&... args) {
        auto session_id = Send<MsgT>(target, std::forward<Args>(args)...);
        return RecvOrNullWithTimeout(session_id.value(), timeout_s);
    }

    void Timeout(uint32_t tick, MessagePointer msg);

    template <typename MsgT, typename ...Args>
    void Timeout(uint32_t tick, Args&&... args) {
        Timeout(tick, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

public:
    IMillion& imillion() { return *imillion_; }
    Logger& logger();
    const ServiceHandle& service_handle() const { return service_handle_; }
    ServiceId service_id() const { return service_id_; }

protected:
    virtual bool OnInit() { return true; }
    virtual Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) { co_return nullptr; }
    virtual Task<MessagePointer> OnMsg(ServiceHandle sender, SessionId session_id, MessagePointer msg) { co_return co_await MessageDispatch(std::move(sender), session_id, std::move(msg)); }
    virtual Task<MessagePointer> OnStop(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) { co_return nullptr; }
    virtual void OnExit() { }


    virtual ServiceTypeKey GetTypeKey() = 0;

    Task<MessagePointer> MessageDispatch(ServiceHandle sender, SessionId session_id, MessagePointer msg) {
        auto service_type_key = GetTypeKey();
        auto msg_type_key = msg.GetTypeKey();
        auto it = msg_handlers_.find({service_type_key, msg_type_key});
        if (it != msg_handlers_.end()) {
            co_return co_await it->second(this, std::move(sender), session_id, std::move(msg));
        }
        co_return nullptr;
    }

    template <typename MsgT, typename ServiceT>
    static void BindMsgHandler(Task<MessagePointer>(ServiceT::* handler)(const ServiceHandle&, SessionId, MessagePointer, MsgT*)) {
        msg_handlers_[{ typeid(ServiceT), GetMessageTypeKey<MsgT>() }] = [handler](IService* iservice, ServiceHandle sender, SessionId session_id, MessagePointer msg_ptr) -> Task<MessagePointer> {
            ServiceT* service = static_cast<ServiceT*>(iservice);
            if constexpr (std::is_const_v<std::remove_pointer_t<MsgT>>) {
                auto* msg = msg_ptr.GetMessage<MsgT>();
                co_return co_await (service->*handler)(std::move(sender), session_id, std::move(msg_ptr), msg);
            }
            else {
                auto* msg = msg_ptr.GetMutableMessage<MsgT>();
                co_return co_await (service->*handler)(std::move(sender), session_id, std::move(msg_ptr), msg);
            }
        };
    }

    template <typename MsgT, typename ServiceT>
    static void RemoveMsgHandler(Task<MessagePointer>(ServiceT::* handler)(const ServiceHandle&, SessionId, MessagePointer, MsgT*)) {
        msg_handlers_.erase({ typeid(ServiceT), GetMessageTypeKey<MsgT>() });
    }

    //template <typename MsgT, typename ServiceT>
    //static bool AutoRegisterMsgHandler() {
    //    static bool registered = [handler] {
    //        return true;
    //    }();
    //    return registered;
    //}

private:
    void set_service_handle(const ServiceHandle& handle) { service_handle_ = handle; }
    void set_service_id(ServiceId service_id) { service_id_ = service_id; }

private:
    template <typename MsgT, typename ServiceT>
    friend class AutoRegisterMsgHandler;
    friend class ServiceCore;
    friend class ServiceMgr;

    IMillion* imillion_;
    ServiceId service_id_;
    ServiceHandle service_handle_;

    struct TypeIndexPair {
        ServiceTypeKey service_type_key;
        MessageTypeKey msg_type_key;

        bool operator==(const TypeIndexPair& other) const {
            return service_type_key == other.service_type_key && msg_type_key == other.msg_type_key;
        }
    };

    struct TypeIndexPairHash {
        size_t operator()(const TypeIndexPair& p) const {
            return p.service_type_key.hash_code() ^ p.msg_type_key;
        }
    };

    using MsgHandlerMap = std::unordered_map<TypeIndexPair
        , std::function<Task<MessagePointer>(IService*, ServiceHandle, SessionId, MessagePointer)>
        , TypeIndexPairHash>;

    static inline MsgHandlerMap msg_handlers_;
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


#define MILLION_SERVICE_DEFINE(SERVICE_CLASS_) \
    private:\
        using SELF_CLASS_ = SERVICE_CLASS_; \
    virtual ::million::ServiceTypeKey GetTypeKey() override { return typeid(SELF_CLASS_); }
    

#define MILLION_PRIMITIVE_CAT(A_, B_) A_##B_
#define MILLION_CAT(NAME_, LINE_) MILLION_PRIMITIVE_CAT(NAME_, LINE_)

#define MILLION_MESSAGE_HANDLE(MSG_TYPE_, MSG_NAME_) \
    static inline auto MILLION_CAT(MILLION_CAT(on_handle_, __LINE__), _) = ::million::AutoRegisterMsgHandler<MSG_TYPE_, SELF_CLASS_>(); \
    ::million::Task<::million::MessagePointer> OnHandle(const ::million::ServiceHandle& sender, ::million::SessionId session_id, ::million::MessagePointer msg_, MSG_TYPE_* MSG_NAME_)

#define MILLION_MESSAGE_HANDLE_IMPL(CLASS_NAME_, MSG_TYPE_, MSG_NAME_) \
    ::million::Task<::million::MessagePointer> CLASS_NAME_::OnHandle(const ::million::ServiceHandle& sender, ::million::SessionId session_id, ::million::MessagePointer msg_, MSG_TYPE_* MSG_NAME_)

} // namespace million