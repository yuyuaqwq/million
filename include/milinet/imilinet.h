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
    virtual ServiceId CreateService(std::unique_ptr<IService> iservice) = 0;

    template <typename IServiceT, typename ...Args>
    ServiceId CreateService(Args&&... args) {
        auto iservice = std::make_unique<IServiceT>(this, std::forward<Args>(args)...);
        return CreateService(std::move(iservice));
    }

    virtual SessionId Send(ServiceId service_id, MsgUnique msg) = 0;
};

inline SessionId IService::Send(ServiceId target_id, MsgUnique msg) {
    return imilinet_->Send(target_id, std::move(msg));
}

} // namespace milinet