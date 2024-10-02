#pragma once

#include <cstdint>

#include <memory>
#include <mutex>
#include <queue>

#include "noncopyable.h"

#include "milinet/service_def.h"
#include "milinet/msg.h"
#include "milinet/msg_executor.h"

namespace milinet {

class ServiceMgr;
class Service : noncopyable {
public:
    Service(ServiceMgr* mgr, ServiceId service_id);
    virtual ~Service();

    void PushMsg(MsgUnique msg);
    MsgUnique PopMsg();
    bool MsgQueueEmpty();

    bool ProcessMsg();
    void ProcessMsgs(size_t count);

    virtual Task OnMsg(MsgUnique msg);

    template <typename MsgT, typename ...Args>
    SessionId Send(ServiceId id, Args&&... args);

    template <typename MsgT = Msg>
    Awaiter<MsgT> Recv(SessionId session_id) {
        return Awaiter<MsgT>(session_id);
    }

    ServiceId service_id() const { return service_id_; }
    bool in_queue() const { return in_queue_; }
    void set_in_queue(bool in_queue) { in_queue_ = in_queue; }

private:
    ServiceMgr* mgr_;

    ServiceId service_id_;

    std::mutex msgs_mutex_;
    std::queue<MsgUnique> msgs_;

    MsgExecutor excutor_;

    bool in_queue_ = false;
};

} // namespace milinet