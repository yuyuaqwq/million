#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <optional>

#include <million/detail/noncopyable.h>

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

    void AddTask(ServiceHandle service, uint32_t tick, MsgUnique msg);

private:
    Million* million_;
    std::optional<std::jthread> thread_;
    detail::HeapTimer tasks_;
    std::atomic_bool run_ = false;
};

} // namespace million