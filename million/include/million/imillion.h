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

class Million;
class MILLION_CLASS_API IMillion : noncopyable {
public:
    IMillion();
    virtual ~IMillion();

    bool Init(std::string_view config_path);

    std::optional<ServiceHandle> AddService(std::unique_ptr<IService> iservice);
    template <typename IServiceT, typename ...Args>
    std::optional<ServiceHandle> NewService(Args&&... args) {
        auto iservice = std::make_unique<IServiceT>(this, std::forward<Args>(args)...);
        return AddService(std::move(iservice));
    }
    void DeleteService(ServiceHandle&& service_handle);
    bool SetServiceUniqueName(const ServiceHandle& handle, const ServiceUniqueName& unique_name);
    std::optional<ServiceHandle> GetServiceByUniqueNum(const ServiceUniqueName& unique_name);

    SessionId Send(const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg);
    SessionId Send(SessionId session_id, const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg);
    template <typename MsgT, typename ...Args>
    SessionId Send(const ServiceHandle& sender, const ServiceHandle& target, Args&&... args) {
        return Send(sender, target, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

    const YAML::Node& YamlConfig() const;
    void Timeout(uint32_t tick, const ServiceHandle& service, MsgUnique msg);
    asio::io_context& NextIoContext();
    void Log(const ServiceHandle& sender, logger::LogLevel level, const char* file, int line, const char* function, const std::string& str);
    void EnableSeparateWorker(const ServiceHandle& service);

private:

    Million* million_ = nullptr;
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

} // namespace million