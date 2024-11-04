#include "session_monitor.h"

#include <iostream>
#include <chrono>

#include "million.h"
#include "service_mgr.h"
#include "service.h"
#include "million_msg.h"

namespace million {

SessionMonitor::SessionMonitor(Million* million, uint32_t tick_s, uint32_t timeout_s)
    : million_(million)
    , timeout_s_(timeout_s)
    , tasks_(tick_s * 1000) {}

SessionMonitor::~SessionMonitor() = default;

void SessionMonitor::Start() {
    run_ = true;
    thread_.emplace([this]() {
        tasks_.Init();
        auto timeout = [this](auto&& task) {
            // 发送一条消息给该服务，让服务自己去确定该会话是否已经完成了，没完成则移除并超时告警
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

void SessionMonitor::AddSession(ServiceHandle service, SessionId session_id) {
    tasks_.AddTask(timeout_s_, { service, session_id });
}


} // namespace million