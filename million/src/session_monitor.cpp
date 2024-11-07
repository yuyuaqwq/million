#include "session_monitor.h"

#include <iostream>
#include <chrono>

#include "million.h"
#include "service_mgr.h"
#include "service.h"
#include "million_msg.h"

namespace million {

SessionMonitor::SessionMonitor(Million* million, uint32_t s_per_tick, uint32_t timeout_tick)
    : million_(million)
    , timeout_tick_(timeout_tick)
    , tasks_(s_per_tick * 1000) {}

SessionMonitor::~SessionMonitor() = default;

void SessionMonitor::Start() {
    run_ = true;
    thread_.emplace([this]() {
        tasks_.Init();
        auto timeout = [this](auto&& task) {
            million_->Send<MillionSessionTimeoutMsg>(task.data.service, task.data.service, task.data.session_id);
        };
        while (run_) {
            tasks_.Tick(timeout);
        }
    });
}

void SessionMonitor::Stop() {
    run_ = false;
}

void SessionMonitor::AddSession(const ServiceHandle& service, SessionId session_id) {
    tasks_.AddTask(timeout_tick_, { service, session_id });
}


} // namespace million