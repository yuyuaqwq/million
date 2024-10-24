#include "service.h"

#include <cassert>

#include <million/imsg.h>

#include "service_mgr.h"

namespace million {

Service::Service(std::unique_ptr<IService> iservice)
    : iservice_(std::move(iservice)){}

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
    auto task = iservice_->OnMsg(std::move(msg));
    if (!task.handle.done()) {
        assert(task.handle.promise().get_awaiter());
        excutor_.Push(task.handle.promise().get_awaiter()->get_waiting(), std::move(task));
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

} // namespace million
