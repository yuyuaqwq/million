#pragma once

#include <cstdint>

#include <memory>
#include <mutex>
#include <queue>

#include "noncopyable.h"

#include "milinet/msg.h"
#include "milinet/executor.hpp"
#include "milinet/service_def.h"

namespace milinet {

class Milinet;
class Service : noncopyable {
public:
    Service(Milinet* milinet, ServiceId id);
    virtual ~Service();

    void PushMsg(MsgUnique msg);
    MsgUnique PopMsg();
    bool MsgQueueEmpty();

    bool ProcessMsg();
    void ProcessMsgs(size_t count);

    virtual Task OnMsg(MsgUnique msg);

    void Send(ServiceId id, MsgUnique msg);
    Awaiter Recv(SessionId session_id);

    ServiceId id() const { return id_; }
    bool in_queue() const { return in_queue_; }
    void set_in_queue(bool in_queue) { in_queue_ = in_queue; }

private:
    Milinet* milinet_;

    ServiceId id_;

    std::mutex msgs_mutex_;
    std::queue<MsgUnique> msgs_;

    ServiceMsgExecutor excutor_;

    bool in_queue_ = false;
};

} // namespace milinet