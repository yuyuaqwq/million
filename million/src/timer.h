#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <optional>

#include <million/noncopyable.h>
#include <million/service_handle.h>
#include <million/msg_def.h>

#include "detail/wheel_timer.hpp"
#include "detail/heap_timer.hpp"

namespace million {

class Million;
class Timer : noncopyable {
public:
    Timer(Million* million, uint32_t ms_per_tick);
    ~Timer();

    void Start();
    void Stop();

    void AddTask(uint32_t tick, ServiceHandle service, MsgUnique msg);

private:
    Million* million_;
    std::optional<std::jthread> thread_;
    struct TimedMsg {
        ServiceHandle service;
        MsgUnique msg;
    };
    detail::HeapTimer<TimedMsg> tasks_;
    // detail::WheelTimer<TimedMsg> tasks_;
    std::atomic_bool run_ = false;
};

} // namespace million