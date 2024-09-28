#include "milinet/service.h"

#include "milinet/msg.h"

namespace milinet {

Service::Service(uint32_t id) : id_(id) {};
Service::~Service() = default;

void Service::PushMsg(std::unique_ptr<Msg> msg) {
    std::unique_lock<std::mutex> guard(msgs_mutex_);
    msgs_.emplace(std::move(msg));
}

std::unique_ptr<Msg> Service::PopMsg() {
    std::unique_lock<std::mutex> guard(msgs_mutex_);
    if (msgs_.empty()) {
        return nullptr;
    }
    auto msg = std::move(msgs_.front());
    msgs_.pop();
    return msg;
}

bool Service::MsgQueueEmpty() {
    std::unique_lock<std::mutex> guard(msgs_mutex_);
    return msgs_.empty();
}

bool Service::ProcessMsg() {
    auto msg = PopMsg();
    if (!msg) {
        return false;
    }
    OnMsg(std::move(msg));
    return true;
}

void Service::ProcessMsgs(size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (!ProcessMsg()) {
            break;
        }
    }
}

void Service::OnMsg(std::unique_ptr<Msg> msg) {}

} // namespace milinet
