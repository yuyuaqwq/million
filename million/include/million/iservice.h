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

class MessageElementWithWeakSender : public noncopyable {
public:
    MessageElementWithWeakSender(ServiceHandle sender, SessionId session_id, MessagePointer message) :
        sender_(std::move(sender)),
        session_id_(session_id),
        message_(std::move(message)) {}
    ~MessageElementWithWeakSender() = default;

    MessageElementWithWeakSender(MessageElementWithWeakSender&&) noexcept = default;
    MessageElementWithWeakSender& operator=(MessageElementWithWeakSender&&) noexcept = default;

    const ServiceHandle& sender() const { return sender_; }
    SessionId session_id() const { return session_id_; }
    const MessagePointer& message() const { return message_; }
    MessagePointer& message() { return message_; }

private:
    ServiceHandle sender_;
    SessionId session_id_;
    MessagePointer message_;
};

class MessageElementWithStrongSender : public noncopyable {
public:
    MessageElementWithStrongSender(ServiceShared sender, SessionId session_id, MessagePointer message) :
        sender_(std::move(sender)),
        session_id_(session_id),
        message_(std::move(message)) {}
    ~MessageElementWithStrongSender() = default;

    MessageElementWithStrongSender(MessageElementWithStrongSender&&) noexcept = default;
    MessageElementWithStrongSender& operator=(MessageElementWithStrongSender&&) noexcept = default;

    const ServiceShared& sender() const { return sender_; }
    SessionId session_id() const { return session_id_; }
    const MessagePointer& message() const { return message_; }
    MessagePointer& message() { return message_; }

private:
    ServiceShared sender_;
    SessionId session_id_;
    MessagePointer message_;
};

template <typename MessageT, typename ServiceT>
class AutoRegisterMessageHandler {
public:
    using OnHandlePointer
        = Task<MessagePointer>(ServiceT::*)(const ServiceHandle&, SessionId, MessagePointer, MessageT*);

    AutoRegisterMessageHandler() {
        handler_ = &ServiceT::OnHandle;
        ServiceT:: BindMessageHandler(handler_);
    }

    ~AutoRegisterMessageHandler() {
        ServiceT:: RemoveMessageHandler(handler_);
    }

private:
    OnHandlePointer handler_;
};

class IMillion;
class MILLION_API IService : public noncopyable {
public:
    IService(IMillion* imillion)
        : imillion_(imillion) {}
    virtual ~IService() = default;

public:
    std::optional<SessionId> Send(const ServiceHandle& target, MessagePointer msg);

    template <typename MessageT, typename ...Args>
    std::optional<SessionId> Send(const ServiceHandle& target, Args&&... args) {
        return Send(target, make_message<MessageT>(std::forward<Args>(args)...));
    }

    bool SendTo(const ServiceHandle& target, SessionId session_id, MessagePointer msg);

    template <typename MessageT, typename ...Args>
    bool SendTo(const ServiceHandle& target, SessionId session_id, Args&&... args) {
        return SendTo(target, session_id, make_message<MessageT>(std::forward<Args>(args)...));
    }

    bool Reply(const ServiceHandle& target, SessionId session_id, MessagePointer msg);

    template <typename MessageT, typename ...Args>
    bool Reply(const ServiceHandle& target, SessionId session_id, Args&&... args) {
        return Reply(target, session_id, make_message<MessageT>(std::forward<Args>(args)...));
    }

    SessionAwaiterBase Recv(SessionId session_id) {
        return SessionAwaiterBase(session_id, 0, false);
    }

    template <typename MessageT>
    SessionAwaiter<MessageT> Recv(SessionId session_id) {
        // 0表示默认超时时间
        return SessionAwaiter<MessageT>(session_id, 0, false);
    }

    SessionAwaiterBase RecvWithTimeout(SessionId session_id, uint32_t timeout_s) {
        return SessionAwaiterBase(session_id, timeout_s, false);
    }

    template <typename MessageT>
    SessionAwaiter<MessageT> RecvWithTimeout(SessionId session_id, uint32_t timeout_s) {
        return SessionAwaiter<MessageT>(session_id, timeout_s, false);
    }

    SessionAwaiterBase RecvOrNull(SessionId session_id) {
        return SessionAwaiterBase(session_id, 0, true);
    }

    template <typename MessageT>
    SessionAwaiter<MessageT> RecvOrNull(SessionId session_id) {
        // 0表示默认超时时间
        return SessionAwaiter<MessageT>(session_id, 0, true);
    }

    SessionAwaiterBase RecvOrNullWithTimeout(SessionId session_id, uint32_t timeout_s) {
        return SessionAwaiterBase(session_id, timeout_s, true);
    }

    template <typename MessageT>
    SessionAwaiter<MessageT> RecvOrNullWithTimeout(SessionId session_id, uint32_t timeout_s) {
        return SessionAwaiter<MessageT>(session_id, timeout_s, true);
    }


    template <typename SendMessageT, typename RecvMessageT, typename ...SendArgsT>
    SessionAwaiter<RecvMessageT> Call(const ServiceHandle& target, SendArgsT&&... args) {
        auto session_id = Send<SendMessageT>(target, std::forward<SendArgsT>(args)...);
        return Recv<RecvMessageT>(session_id.value());
    }

    template <typename MessageT, typename ...Args>
    SessionAwaiterBase Call(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send<MessageT>(target, std::forward<Args>(args)...);
        return Recv(session_id.value());
    }

    template <typename SendMessageT, typename RecvMessageT, typename ...SendArgsT>
    SessionAwaiter<RecvMessageT> CallWithTimeout(const ServiceHandle& target, uint32_t timeout_s, SendArgsT&&... args) {
        auto session_id = Send<SendMessageT>(target, std::forward<SendArgsT>(args)...);
        return RecvWithTimeout<RecvMessageT>(session_id.value(), timeout_s);
    }

    template <typename MessageT, typename ...Args>
    SessionAwaiterBase CallWithTimeout(const ServiceHandle& target, uint32_t timeout_s, Args&&... args) {
        auto session_id = Send<MessageT>(target, std::forward<Args>(args)...);
        return RecvWithTimeout(session_id.value(), timeout_s);
    }

    template <typename SendMessageT, typename RecvMessageT, typename ...SendArgsT>
    SessionAwaiter<RecvMessageT> CallOrNull(const ServiceHandle& target, SendArgsT&&... args) {
        auto session_id = Send<SendMessageT>(target, std::forward<SendArgsT>(args)...);
        return RecvOrNull<RecvMessageT>(session_id.value());
    }

    template <typename MessageT, typename ...Args>
    SessionAwaiterBase CallOrNull(const ServiceHandle& target, Args&&... args) {
        auto session_id = Send<MessageT>(target, std::forward<Args>(args)...);
        return RecvOrNull(session_id.value());
    }

    template <typename SendMessageT, typename RecvMessageT, typename ...SendArgsT>
    SessionAwaiter<RecvMessageT> CallOrNullWithTimeout(const ServiceHandle& target, uint32_t timeout_s, SendArgsT&&... args) {
        auto session_id = Send<SendMessageT>(target, std::forward<SendArgsT>(args)...);
        return RecvOrNullWithTimeout<RecvMessageT>(session_id.value(), timeout_s);
    }

    template <typename MessageT, typename ...Args>
    SessionAwaiterBase CallOrNullWithTimeout(const ServiceHandle& target, uint32_t timeout_s, Args&&... args) {
        auto session_id = Send<MessageT>(target, std::forward<Args>(args)...);
        return RecvOrNullWithTimeout(session_id.value(), timeout_s);
    }

    void Timeout(uint32_t tick, MessagePointer msg);

    template <typename MessageT, typename ...Args>
    void Timeout(uint32_t tick, Args&&... args) {
        Timeout(tick, std::make_unique<MessageT>(std::forward<Args>(args)...));
    }

public:
    IMillion& imillion() { return *imillion_; }
    Logger& logger();
    const ServiceHandle& service_handle() const { return service_handle_; }
    ServiceId service_id();

protected:
    using MessageHandler = std::function<Task<MessagePointer>(IService*, ServiceHandle, SessionId, MessagePointer)>;

    virtual bool OnInit() { return true; }
    virtual Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) { co_return nullptr; }
    virtual Task<MessagePointer> OnMsg(ServiceHandle sender, SessionId session_id, MessagePointer msg) { co_return co_await MessageDispatch(std::move(sender), session_id, std::move(msg)); }
    virtual Task<MessagePointer> OnStop(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) { co_return nullptr; }
    virtual void OnExit() { }

    virtual ServiceTypeKey GetTypeKey() = 0;

    Task<MessagePointer> MessageDispatch(ServiceHandle sender, SessionId session_id, MessagePointer msg) {
        auto handler = FindMessageHandler(msg);
        if (handler) {
            co_return co_await (*handler)(this, std::move(sender), session_id, std::move(msg));
        }
        co_return nullptr;
    }

    MessageHandler* FindMessageHandler(const MessagePointer& msg) {
        auto service_type_key = GetTypeKey();
        auto msg_type_key = msg.GetTypeKey();
        auto it = message_handlers_.find({ service_type_key, msg_type_key });
        if (it != message_handlers_.end()) {
            return &it->second;
        }
        return nullptr;
    }

    template <typename MessageT, typename ServiceT>
    static void BindMessageHandler(Task<MessagePointer>(ServiceT::* handler)(const ServiceHandle&, SessionId, MessagePointer, MessageT*)) {
        message_handlers_[{ typeid(ServiceT), GetMessageTypeKey<MessageT>() }] = [handler](IService* iservice, ServiceHandle sender, SessionId session_id, MessagePointer msg_ptr) -> Task<MessagePointer> {
            ServiceT* service = static_cast<ServiceT*>(iservice);
            if constexpr (std::is_const_v<std::remove_pointer_t<MessageT>>) {
                auto* msg = msg_ptr.GetMessage<MessageT>();
                co_return co_await (service->*handler)(std::move(sender), session_id, std::move(msg_ptr), msg);
            }
            else {
                auto* msg = msg_ptr.GetMutableMessage<MessageT>();
                co_return co_await (service->*handler)(std::move(sender), session_id, std::move(msg_ptr), msg);
            }
        };
    }

    template <typename MessageT, typename ServiceT>
    static void RemoveMessageHandler(Task<MessagePointer>(ServiceT::* handler)(const ServiceHandle&, SessionId, MessagePointer, MessageT*)) {
        message_handlers_.erase({ typeid(ServiceT), GetMessageTypeKey<MessageT>() });
    }

    //template <typename MessageT, typename ServiceT>
    //static bool AutoRegisterMsgHandler() {
    //    static bool registered = [handler] {
    //        return true;
    //    }();
    //    return registered;
    //}

private:
    void set_service_handle(const ServiceHandle& handle) { service_handle_ = handle; }
    void set_service_shared(const ServiceShared& service_shared) { service_shared_ = service_shared; }

private:
    template <typename MessageT, typename ServiceT>
    friend class AutoRegisterMessageHandler;
    friend class ServiceCore;
    friend class ServiceMgr;

    IMillion* imillion_;
    ServiceShared service_shared_;
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

    using MessageHandlerMap = std::unordered_map<TypeIndexPair, MessageHandler, TypeIndexPairHash>;

    static inline MessageHandlerMap message_handlers_;
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
//            if (::million::GetMessageTypeKey<STOP_MSG_TYPE_>()) { \
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
    static inline auto MILLION_CAT(MILLION_CAT(on_handle_, __LINE__), _) = ::million::AutoRegisterMessageHandler<MSG_TYPE_, SELF_CLASS_>(); \
    ::million::Task<::million::MessagePointer> OnHandle(const ::million::ServiceHandle& sender, ::million::SessionId session_id, ::million::MessagePointer msg_, MSG_TYPE_* MSG_NAME_)

#define MILLION_MESSAGE_HANDLE_IMPL(CLASS_NAME_, MSG_TYPE_, MSG_NAME_) \
    ::million::Task<::million::MessagePointer> CLASS_NAME_::OnHandle(const ::million::ServiceHandle& sender, ::million::SessionId session_id, ::million::MessagePointer msg_, MSG_TYPE_* MSG_NAME_)

} // namespace million