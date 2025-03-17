#include "session_monitor.h"

#include <iostream>
#include <chrono>

#include "million.h"
#include "service_mgr.h"
#include "service_impl.h"

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
            million_->Send(task.data.service, task.data.service, make_cpp_msg<SessionTimeoutMsg>(task.data.session_id));
        };
        while (run_) {
            tasks_.Tick(timeout);
        }

        // 是否将所有task都发送超时？
    });
}

void SessionMonitor::Stop() {
    run_ = false;
    thread_.reset();
}

void SessionMonitor::AddSession(const ServiceShared& service, SessionId session_id, uint32_t timeout_s) {
    if (timeout_s == 0) {
        timeout_s = timeout_tick_;
    }
    tasks_.AddTask(timeout_s, { service, session_id });
}


} // namespace million