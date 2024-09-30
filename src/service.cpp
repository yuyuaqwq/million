#include <cassert>

#include "milinet/service.h"

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
    auto op_msg = excutor_.TrySchedule(session_id, std::move(msg));
    if (!op_msg) {
        return true;
    }
    msg = std::move(*op_msg);
    auto co = OnMsg(std::move(msg));
    if (!co.handle.done()) {
        excutor_.Push(co.handle.promise().get_waiting(), std::move(co));
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
