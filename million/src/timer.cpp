#include "timer.h"

#include <iostream>
#include <chrono>

#include "million.h"
#include "service_mgr.h"
#include "service.h"

namespace million {

Timer::Timer(Million* million, uint32_t ms_per_tick)
    : million_(million)
    , tasks_(ms_per_tick) {}

Timer::~Timer() = default;

void Timer::Start() {
    run_ = true;
    thread_.emplace([this]() {
        tasks_.Init();
        auto send = [this](auto&& task) {
            million_->Send(task.data.service, task.data.service, std::move(task.data.msg));
        };
        while (run_) {
            tasks_.Tick(send);
        }
    });
}

void Timer::Stop() {
    run_ = false;
}

void Timer::AddTask(uint32_t tick, const ServiceHandle& service, MsgUnique msg) {
    tasks_.AddTask(tick, { service, std::move(msg) });
}

} // namespace million