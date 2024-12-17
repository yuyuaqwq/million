#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <optional>

#include <million/noncopyable.h>
#include <million/service_handle.h>
#include <million/session_def.h>
#include <million/cpp_msg.h>

#include "detail/wheel_timer.hpp"

namespace million {

MILLION_MSG_DEFINE(, SessionTimeoutMsg, (SessionId) timeout_id);

class Million;
class SessionMonitor : noncopyable {
public:
    SessionMonitor(Million* million, uint32_t s_per_tick, uint32_t timeout_tick);
    ~SessionMonitor();

    void Start();
    void Stop();

    void AddSession(const ServiceHandle& service, SessionId session_id, uint32_t timeout_s);
private:
    Million* million_;
    uint32_t timeout_tick_;

    std::optional<std::jthread> thread_;
    struct TimedMsg {
        ServiceHandle service;
        SessionId session_id;
    };
    detail::WheelTimer<TimedMsg> tasks_;
    std::atomic_bool run_ = false;
};

} // namespace million