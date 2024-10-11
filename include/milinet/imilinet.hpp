#pragma once

#include <string_view>

#include <milinet/detail/dl_export.hpp>
#include <milinet/detail/noncopyable.h>
#include <milinet/msg_def.h>
#include <milinet/iservice.hpp>

namespace YAML {

class Node;

} // namespace YAML

namespace milinet {

class MILINET_CLASS_EXPORT IMilinet : noncopyable {
public:
    virtual ~IMilinet() = default;

    virtual ServiceHandle AddService(std::unique_ptr<IService> iservice) = 0;

    template <typename IServiceT, typename ...Args>
    ServiceHandle MakeService(Args&&... args) {
        auto iservice = std::make_unique<IServiceT>(this, std::forward<Args>(args)...);
        return AddService(std::move(iservice));
    }

    virtual SessionId Send(ServiceHandle target, MsgUnique msg) = 0;
    template <typename MsgT, typename ...Args>
    SessionId Send(ServiceHandle target, Args&&... args) {
        return Send(target, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

    virtual const YAML::Node& config() const = 0;
};

inline SessionId IService::Send(ServiceHandle target, MsgUnique msg) {
    return imilinet_->Send(target, std::move(msg));
}

MILINET_FUNC_EXPORT IMilinet* NewMilinet(std::string_view config_path);

MILINET_FUNC_EXPORT void DeleteMilinet(IMilinet* milinet);

} // namespace milinet