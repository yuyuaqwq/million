#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <optional>

#include <million/noncopyable.h>
#include <million/service_handle.h>
#include <million/msg_def.h>

#include "detail/wheel_timer.hpp"

namespace million {

class Million;
class SessionMonitor : noncopyable {
public:
    SessionMonitor(Million* million, uint32_t s_per_tick, uint32_t timeout_tick);
    ~SessionMonitor();

    void Start();
    void Stop();

    void AddSession(ServiceHandle service, SessionId session_id);
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