#pragma once

#include <memory>

#include "milinet/noncopyable.h"
#include "milinet/service_def.h"
#include "milinet/msg.h"
#include "milinet/task.hpp"

namespace milinet {

class IMilinet;
class IService : noncopyable {
public:
    IService(IMilinet* imilinet)
        : imilinet_(imilinet) {}
    virtual ~IService() = default;

    SessionId Send(ServiceHandle handle, MsgUnique msg);

    template <typename MsgT, typename ...Args>
    SessionId Send(ServiceHandle handle, Args&&... args) {
        return Send(handle, std::make_unique<MsgT>(std::forward<Args>(args)...));
    }

    template <typename MsgT>
    Awaiter<MsgT> Recv(SessionId session_id) {
        return Awaiter<MsgT>(session_id);
    }

    virtual Task OnMsg(MsgUnique msg) = 0;

    ServiceHandle service_handle() const { return service_handle_; }
    void set_service_handle(ServiceHandle handle) { service_handle_ = handle; }

private:
    IMilinet* imilinet_;
    ServiceHandle service_handle_;
};

} // namespace milinet