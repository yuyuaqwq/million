#include "timer.h"

#include <iostream>
#include <chrono>

#include "million.h"
#include "service_mgr.h"
#include "service.h"

namespace million {

Timer::Timer(Million* million, uint32_t ms_per_tick)
    : million_(million)
    , tasks_(million, ms_per_tick) {}

Timer::~Timer() = default;

void Timer::Start() {
    run_ = true;
    thread_.emplace([this]() {
        tasks_.Init();
        while (run_) {
            tasks_.Tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(tasks_.ms_per_tick()));
        }
    });
}

void Timer::Stop() {
    run_ = false;
}

void Timer::AddTask(ServiceHandle service, uint32_t tick, MsgUnique msg) {
    tasks_.AddTask(service , tick, std::move(msg));
}

} // namespace million