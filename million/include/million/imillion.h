#pragma once

#include <string_view>

#include <million/api.h>
#include <million/noncopyable.h>
#include <million/imsg.h>
#include <million/iservice.h>

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

    virtual SessionId Send(ServiceHandle sender, ServiceHandle target, MsgUnique msg) = 0;
    virtual SessionId Send(SessionId session_id, ServiceHandle sender, ServiceHandle target, MsgUnique msg) = 0;
    template <typename MsgT, typename ...Args>
    SessionId Send(ServiceHandle sender, ServiceHandle target, Args&&... args) {
        return Send(sender, target, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

    virtual void Timeout(uint32_t tick, ServiceHandle service, MsgUnique msg) = 0;

    virtual asio::io_context& NextIoContext() = 0;

    virtual const YAML::Node& YamlConfig() const = 0;
};

inline SessionId IService::Send(ServiceHandle target, MsgUnique msg) {
    return imillion_->Send(service_handle_, target, std::move(msg));
}

inline void IService::Reply(MsgUnique msg) {
    auto target = msg->sender();
    auto session_id = msg->session_id();
    imillion_->Send(session_id, service_handle_, target, std::move(msg));
}

inline void IService::Reply(ServiceHandle target, SessionId session_id) {
    Reply<IMsg>(target, session_id);
}

inline void IService::Timeout(uint32_t tick, MsgUnique msg) {
    imillion_->Timeout(tick, service_handle(), std::move(msg));
}

MILLION_FUNC_API void InitMillion();
MILLION_FUNC_API IMillion* NewMillion(std::string_view config_path);
MILLION_FUNC_API void DeleteMillion(IMillion* million);

} // namespace million