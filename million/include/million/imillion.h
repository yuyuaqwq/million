#pragma once

#include <string_view>

#include <million/api.h>
#include <million/exception.h>
#include <million/noncopyable.h>
#include <million/msg.h>
#include <million/iservice.h>
#include <million/logger.h>
#include <million/proto_mgr.h>

#ifdef WIN32
#undef StartService
#endif

namespace YAML {

class Node;

} // namespace YAML

namespace asio {

class io_context;

} // namespace asio

namespace million {

class Million;
class MILLION_API IMillion : noncopyable {
public:
    IMillion();
    virtual ~IMillion();

    bool Init(std::string_view settings_path);
    void Start();

    template <typename IServiceT, typename ...Args>
    std::optional<ServiceHandle> NewService(Args&&... args) {
        auto iservice = std::make_unique<IServiceT>(this, std::forward<Args>(args)...);
        auto handle = AddService(std::move(iservice));
        if (!handle) return std::nullopt;
        StartService(*handle, nullptr);
        return handle;
    }

    template <typename IServiceT, typename ...Args>
    std::optional<ServiceHandle> NewServiceWithoutStart(Args&&... args) {
        auto iservice = std::make_unique<IServiceT>(this, std::forward<Args>(args)...);
        return AddService(std::move(iservice));
    }

    std::optional<SessionId> StartService(const ServiceHandle& service, MsgPtr with_msg);
    template <typename MsgT, typename ...Args>
    std::optional<SessionId> StartService(const ServiceHandle& service, Args&&... args) {
        return StartService(service, make_msg<MsgT>(std::forward<Args>(args)...));
    }

    std::optional<SessionId> StopService(const ServiceHandle& service, MsgPtr with_msg);
    template <typename MsgT, typename ...Args>
    std::optional<SessionId> StopService(const ServiceHandle& service, Args&&... args) {
        return StopService(service, make_msg<MsgT>(std::forward<Args>(args)...));
    }

    template <typename RecvMsgT, typename SendMsgT, typename ...Args>
    SessionAwaiter<RecvMsgT> StartServiceSync(const ServiceHandle& service, Args&&... args) {
        auto session_id = StartService<SendMsgT>(service, std::forward<Args>(args)...);
        return Recv<RecvMsgT>(session_id.value());
    }
    template <typename RecvMsgT, typename SendMsgT, typename ...Args>
    SessionAwaiter<RecvMsgT> StopServiceSync(const ServiceHandle& service, Args&&... args) {
        auto session_id = StopService<SendMsgT>(service, std::forward<Args>(args)...);
        return Recv<RecvMsgT>(session_id.value());
    }

    bool SetServiceName(const ServiceHandle& service, const ServiceName& name);
    std::optional<ServiceHandle> GetServiceByName(const ServiceName& name);

    SessionId NewSession();

    std::optional<SessionId> Send(const ServiceHandle& sender, const ServiceHandle& target, MsgPtr msg);
    template <typename MsgT, typename ...Args>
    std::optional<SessionId> Send(const ServiceHandle& sender, const ServiceHandle& target, Args&&... args) {
        return Send(sender, target, make_msg<MsgT>(std::forward<Args>(args)...));
    }

    bool SendTo(const ServiceHandle& sender, const ServiceHandle& target, SessionId session_id, MsgPtr msg);
    template <typename MsgT, typename ...Args>
    bool SendTo(const ServiceHandle& sender, const ServiceHandle& target, SessionId session_id, Args&&... args) {
        return SendTo(sender, target, session_id, make_msg<MsgT>(std::forward<Args>(args)...));
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

    bool Timeout(uint32_t tick, const ServiceHandle& service, MsgPtr msg);
    template <typename MsgT, typename ...Args>
    std::optional<SessionId> Timeout(uint32_t tick, const ServiceHandle& service, Args&&... args) {
        return Timeout(tick, service, make_msg<MsgT>(std::forward<Args>(args)...));
    }

    void EnableSeparateWorker(const ServiceHandle& service);

    const YAML::Node& YamlSettings() const;
    asio::io_context& NextIoContext();
    
    Logger& logger();
    ProtoMgr& proto_mgr();
    Million& impl() { return *impl_; }

protected:
    friend class Million;
    virtual bool OnInit() { return true; }
    virtual void OnExit() { }

private:
    std::optional<ServiceHandle> AddService(std::unique_ptr<IService> iservice);

private:
    Million* impl_;
};

inline std::optional<SessionId> IService::Send(const ServiceHandle& target, MsgPtr msg) {
    return imillion_->Send(service_handle_, target, std::move(msg));
}

inline bool IService::SendTo(const ServiceHandle& target, SessionId session_id, MsgPtr msg) {
    return imillion_->SendTo(service_handle_, target, session_id, std::move(msg)) != kSessionIdInvalid;
}

inline bool IService::Reply(const ServiceHandle& target, SessionId session_id, MsgPtr msg) {
    return imillion_->SendTo(service_handle_, target, SessionSendToReplyId(session_id), std::move(msg)) != kSessionIdInvalid;
}

inline void IService::Timeout(uint32_t tick, MsgPtr msg) {
    imillion_->Timeout(tick, service_handle(), std::move(msg));
}

inline Logger& IService::logger() {
    return imillion_->logger();
}

extern "C" MILLION_API void MillionInit();

extern "C" MILLION_API void* MillionMemAlloc(size_t size);

extern "C" MILLION_API void MillionMemFree(void* ptr);

#define MILLION_MODULE_MEM_INIT() \
    void* operator new(std::size_t size) { \
        void* ptr = ::million::MillionMemAlloc(size); \
        if (!ptr) throw std::bad_alloc(); \
        return ptr; \
    } \
    void operator delete(void* ptr) noexcept { \
        ::million::MillionMemFree(ptr); \
    } \
    void* operator new[](std::size_t size) { \
        void* ptr = ::million::MillionMemAlloc(size); \
        if (!ptr) throw std::bad_alloc(); \
        return ptr; \
    } \
    void operator delete[](void* ptr) noexcept { \
        ::million::MillionMemFree(ptr); \
    } \

#define MILLION_MODULE_INIT() \
    MILLION_MODULE_MEM_INIT();

} // namespace million