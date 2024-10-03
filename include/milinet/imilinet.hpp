#pragma once

#include "milinet/noncopyable.h"
#include "milinet/msg_def.h"
#include "milinet/iservice.hpp"

namespace milinet {

#ifdef __linux__
#define MILINET_FUNC_EXPORT extern "C"
#elif WIN32
#define MILINET_FUNC_EXPORT extern "C" __declspec(dllexport)
#endif

class IMilinet : noncopyable {
public:
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