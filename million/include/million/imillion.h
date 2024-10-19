#pragma once

#include <string_view>

#include <million/detail/dl_export.h>
#include <million/detail/noncopyable.h>
#include <million/imsg.h>
#include <million/iservice.h>

namespace YAML {

class Node;

} // namespace YAML

namespace asio {

class io_context;

} // namespace asio

namespace million {

class MILLION_CLASS_EXPORT IMillion : noncopyable {
public:
    virtual ~IMillion() = default;

    virtual ServiceHandle AddService(std::unique_ptr<IService> iservice) = 0;

    template <typename IServiceT, typename ...Args>
    ServiceHandle NewService(Args&&... args) {
        auto iservice = std::make_unique<IServiceT>(this, std::forward<Args>(args)...);
        return AddService(std::move(iservice));
    }

    virtual SessionId Send(ServiceHandle sender, ServiceHandle target, MsgUnique msg) = 0;
    template <typename MsgT, typename ...Args>
    SessionId Send(ServiceHandle sender, ServiceHandle target, Args&&... args) {
        return Send(sender, target, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

    virtual const YAML::Node& config() const = 0;

    virtual asio::io_context& NextIoContext() = 0;
};

inline SessionId IService::Send(ServiceHandle target, MsgUnique msg) {
    return imillion_->Send(service_handle_, target, std::move(msg));
}

inline void IService::Reply(MsgUnique msg) {
    auto target = msg->sender();
    Send(target, std::move(msg));
}

MILLION_FUNC_EXPORT void InitMillion();
MILLION_FUNC_EXPORT IMillion* NewMillion(std::string_view config_path);
MILLION_FUNC_EXPORT void DeleteMillion(IMillion* million);

} // namespace million