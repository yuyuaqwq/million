#pragma once

#include <string_view>

#include <million/api.h>
#include <million/noncopyable.h>
#include <million/imsg.h>
#include <million/iservice.h>
#include <million/logger/logger.h>

namespace YAML {

class Node;

} // namespace YAML

namespace asio {

class io_context;

} // namespace asio

namespace million {

class MILLION_CLASS_API ConfigException : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message)
        : std::runtime_error("config error: " + message) {}
};

class MILLION_CLASS_API IMillion : noncopyable {
public:
    virtual ~IMillion() = default;

    virtual ServiceHandle AddService(std::unique_ptr<IService> iservice) = 0;

    template <typename IServiceT, typename ...Args>
    ServiceHandle NewService(Args&&... args) {
        auto iservice = std::make_unique<IServiceT>(this, std::forward<Args>(args)...);
        return AddService(std::move(iservice));
    }

    virtual void DeleteService(ServiceHandle&& service_handle) = 0;

    virtual SessionId Send(const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg) = 0;
    virtual SessionId Send(SessionId session_id, const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg) = 0;
    template <typename MsgT, typename ...Args>
    SessionId Send(const ServiceHandle& sender, const ServiceHandle& target, Args&&... args) {
        return Send(sender, target, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

    virtual const YAML::Node& YamlConfig() const = 0;
    virtual void Timeout(uint32_t tick, const ServiceHandle& service, MsgUnique msg) = 0;
    virtual asio::io_context& NextIoContext() = 0;
    virtual void Log(const ServiceHandle& sender, logger::LogLevel level, const char* file, int line, const char* function, std::string_view str) = 0;
    virtual void EnableSeparateWorker(const ServiceHandle& service) = 0;
};

inline SessionId IService::Send(const ServiceHandle& target, MsgUnique msg) {
    return imillion_->Send(service_handle_, target, std::move(msg));
}

inline void IService::Reply(MsgUnique msg) {
    auto target = msg->sender();
    auto session_id = msg->session_id();
    imillion_->Send(session_id, service_handle_, target, std::move(msg));
}

inline void IService::Reply(const ServiceHandle& target, SessionId session_id) {
    Reply<IMsg>(target, session_id);
}

inline void IService::Timeout(uint32_t tick, MsgUnique msg) {
    imillion_->Timeout(tick, service_handle(), std::move(msg));
}

inline void IService::EnableSeparateWorker() {
    imillion_->EnableSeparateWorker(service_handle());
}

MILLION_FUNC_API void InitMillion();
MILLION_FUNC_API IMillion* NewMillion(std::string_view config_path);
MILLION_FUNC_API void DeleteMillion(IMillion* million);

} // namespace million