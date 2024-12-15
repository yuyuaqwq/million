#pragma once

#include <string_view>

#include <million/api.h>
#include <million/noncopyable.h>
#include <million/nonnull_ptr.h>
#include <million/msg.h>
#include <million/iservice.h>
#include <million/logger.h>
#include <million/exception.h>

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

    bool Start(std::string_view config_path);

    std::optional<ServiceHandle> AddService(std::unique_ptr<IService> iservice);
    template <typename IServiceT, typename ...Args>
    std::optional<ServiceHandle> NewService(Args&&... args) {
        auto iservice = std::make_unique<IServiceT>(this, std::forward<Args>(args)...);
        return AddService(std::move(iservice));
    }
    void StopService(const ServiceHandle& service_handle);
    bool SetServiceName(const ServiceHandle& handle, const ServiceName& name);
    std::optional<ServiceHandle> GetServiceByName(const ServiceName& name);

    SessionId NewSession();

    SessionId Send(const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg);
    SessionId SendTo(const ServiceHandle& sender, const ServiceHandle& target, SessionId session_id, MsgUnique msg);
    template <typename MsgT, typename ...Args>
    SessionId SendProtoMsg(const ServiceHandle& sender, const ServiceHandle& target, Args&&... args) {
        return Send(sender, target, make_proto_msg<MsgT>(std::forward<Args>(args)...));
    }
    template <typename MsgT, typename ...Args>
    SessionId SendCppMsg(const ServiceHandle& sender, const ServiceHandle& target, Args&&... args) {
        return Send(sender, target, make_cpp_msg<MsgT>(std::forward<Args>(args)...));
    }

    const YAML::Node& YamlConfig() const;
    void Timeout(uint32_t tick, const ServiceHandle& service, MsgUnique msg);
    asio::io_context& NextIoContext();
    void EnableSeparateWorker(const ServiceHandle& service);

    Logger& logger();

private:

    Million* million_ = nullptr;
};

inline SessionId IService::NewSession() {
    return imillion_->NewSession();
}

inline SessionId IService::Send(const ServiceHandle& target, MsgUnique msg) {
    return imillion_->Send(service_handle_, target, std::move(msg));
}

inline bool IService::SendTo(const ServiceHandle& target, SessionId session_id, MsgUnique msg) {
    return imillion_->SendTo(service_handle_, target, session_id, std::move(msg)) != kSessionIdInvalid;
}

inline void IService::Timeout(uint32_t tick, MsgUnique msg) {
    imillion_->Timeout(tick, service_handle(), std::move(msg));
}

inline void IService::EnableSeparateWorker() {
    imillion_->EnableSeparateWorker(service_handle());
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