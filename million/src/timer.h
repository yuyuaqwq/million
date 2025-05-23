#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <optional>

#include <million/noncopyable.h>
#include <million/service_handle.h>
#include <million/session_def.h>
#include <million/message.h>

#include "internal/wheel_timer.hpp"
#include "internal/heap_timer.hpp"

namespace million {

class Million;
class Timer : noncopyable {
public:
    Timer(Million* million, uint32_t ms_per_tick);
    ~Timer();

    void Start();
    void Stop();

    void AddTask(uint32_t tick, const ServiceShared& service, MessagePointer msg);

private:
    Million* million_;
    std::optional<std::jthread> thread_;
    struct TimedMsg {
        ServiceShared service;
        MessagePointer msg;
    };
    internal::HeapTimer<TimedMsg> tasks_;
    //internal::WheelTimer<TimedMsg> tasks_;
    std::atomic_bool run_ = false;
};

} // namespace million