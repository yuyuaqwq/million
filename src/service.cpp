#include "milinet/service.h"

#include <cassert>

#include "milinet/msg.h"
#include "milinet/milinet.h"

namespace milinet {

Service::Service(Milinet* milinet, ServiceId id) 
    : milinet_(milinet), id_(id) {}

Service::~Service() = default;

void Service::PushMsg(MsgUnique msg) {
    std::lock_guard guard(msgs_mutex_);
    msgs_.emplace(std::move(msg));
}

MsgUnique Service::PopMsg() {
    std::lock_guard guard(msgs_mutex_);
    if (msgs_.empty()) {
        return nullptr;
    }
    auto msg = std::move(msgs_.front());
    msgs_.pop();
    assert(msg);
    return msg;
}

bool Service::MsgQueueEmpty() {
    std::lock_guard guard(msgs_mutex_);
    return msgs_.empty();
}

bool Service::ProcessMsg() {
    auto msg = PopMsg();
    if (!msg) {
        return false;
    }
    auto session_id = msg->session_id();
    msg = excutor_.TrySchedule(session_id, std::move(msg));
    if (!msg) {
        return true;
    }
    auto task = OnMsg(std::move(msg));
    if (!task.handle.done()) {
        excutor_.Push(task.handle.promise().get_waiting(), std::move(task));
    }
    return true;
}

void Service::ProcessMsgs(size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (!ProcessMsg()) {
            break;
        }
    }
}

Task Service::OnMsg(MsgUnique msg) {
    co_return;
}


void Service::Send(ServiceId id, MsgUnique msg) {
    milinet_->Send(id, std::move(msg));
}

} // namespace milinet
