#pragma once

#include <memory>

#include "milinet/noncopyable.h"
#include "milinet/service_def.h"
#include "milinet/msg_def.h"
#include "milinet/task.hpp"

namespace milinet {

class IMilinet;
class IService : noncopyable {
public:
    IService(IMilinet* imilinet)
        : imilinet_(imilinet) {}
    virtual ~IService() = default;

    SessionId Send(ServiceId target_id, MsgUnique msg);

    template <typename MsgT, typename ...Args>
    SessionId Send(ServiceId target_id, Args&&... args) {
        return Send(target_id, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

    template <typename MsgT>
    Awaiter<MsgT> Recv(SessionId session_id) {
        return Awaiter<MsgT>(session_id);
    }

    virtual Task OnMsg(MsgUnique msg) = 0;

    ServiceId service_id() const { return service_id_; }
    void set_service_id(ServiceId service_id) { service_id_ = service_id; }

private:
    IMilinet* imilinet_;
    ServiceId service_id_;
};

} // namespace milinet