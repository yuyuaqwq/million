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
class SessionMonitor : noncopyable {
public:
    SessionMonitor(Million* million, uint32_t ms_per_tick);
    ~SessionMonitor();

    void Start();
    void Stop();

private:
    Million* million_;
    std::optional<std::jthread> thread_;
    detail::HeapTimer tasks_;
    std::atomic_bool run_ = false;
};

} // namespace million