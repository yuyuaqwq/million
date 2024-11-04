#include "service.h"

#include <cassert>

#include <million/imsg.h>

#include "service_mgr.h"
#include "million_msg.h"

namespace million {

Service::Service(ServiceMgr* service_mgr, std::unique_ptr<IService> iservice)
    : service_mgr_(service_mgr)
    , iservice_(std::move(iservice))
    , excutor_(this) {}

Service::~Service() = default;

void Service::PushMsg(MsgUnique msg) {
    std::lock_guard guard(msgs_mutex_);
    msgs_.emplace(std::move(msg));
}

std::optional<MsgUnique> Service::PopMsg() {
    std::lock_guard guard(msgs_mutex_);
    if (msgs_.empty()) {
        return std::nullopt;
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
    auto msg_opt = PopMsg();
    if (!msg_opt) {
        return false;
    }
    auto msg = std::move(*msg_opt);
    if (msg->type() == MillionSessionTimeoutMsg::kType) {
        auto msg_ptr = static_cast<MillionSessionTimeoutMsg*>(msg.get());
        excutor_.TimeoutCleanup(msg_ptr->timeout_id);
        return true;
    }

    auto session_id = msg->session_id();
    msg_opt = excutor_.TrySchedule(session_id, std::move(msg));
    if (!msg_opt) {
        return true;
    }
    auto task = iservice_->OnMsg(std::move(*msg_opt));
    excutor_.AddTask(std::move(task));
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
