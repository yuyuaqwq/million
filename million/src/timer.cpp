#include "timer.h"

#include <iostream>
#include <chrono>

#include "million.h"
#include "service_mgr.h"
#include "service.h"

namespace million {

Timer::Timer(Million* million, uint32_t ms_per_tick)
    : million_(million)
    , time_wheel_(ms_per_tick) {}

Timer::~Timer() = default;

void Timer::Start() {
    run_ = true;
    thread_.emplace([this]() {
        while (run_) {
            time_wheel_.Tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(time_wheel_.ms_per_tick()));
        }
    });
}

void Timer::Stop() {
    run_ = false;
}

void Timer::AddTask(detail::DelayTask&& task) {
    time_wheel_.AddTask(std::move(task));
}

} // namespace million