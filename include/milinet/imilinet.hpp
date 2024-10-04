#pragma once

#include "milinet/detail/dl_export.hpp"
#include "milinet/noncopyable.h"
#include "milinet/msg_def.h"
#include "milinet/iservice.hpp"

namespace milinet {

class MILINET_CLASS_EXPORT IMilinet : noncopyable {
public:
    virtual ~IMilinet() = default;

    virtual ServiceHandle CreateService(std::unique_ptr<IService> iservice) = 0;

    template <typename IServiceT, typename ...Args>
    ServiceHandle CreateService(Args&&... args) {
        auto iservice = std::make_unique<IServiceT>(this, std::forward<Args>(args)...);
        return CreateService(std::move(iservice));
    }

    virtual SessionId Send(ServiceHandle target, MsgUnique msg) = 0;
    template <typename MsgT, typename ...Args>
    SessionId Send(ServiceHandle target, Args&&... args) {
        return Send(target, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }
};

inline SessionId IService::Send(ServiceHandle target, MsgUnique msg) {
    return imilinet_->Send(target, std::move(msg));
}

} // namespace milinet