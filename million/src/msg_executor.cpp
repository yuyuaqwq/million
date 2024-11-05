#include "msg_executor.h"

#include <cassert>

#include <iostream>

#include <million/imsg.h>

#include <million/logger/logger.h>

#include "service.h"
#include "service_mgr.h"
#include "session_monitor.h"
#include "million.h"

namespace million {

MsgExecutor::MsgExecutor(Service* service)
    : service_(service) {}

MsgExecutor::~MsgExecutor() = default;

// 尝试调度
std::optional<MsgUnique> MsgExecutor::TrySchedule(SessionId id, MsgUnique msg) {
    auto iter = tasks_.find(id);
    if (iter == tasks_.end()) {
        return msg;
    }
    auto awaiter = iter->second.coroutine.promise().awaiter();
    awaiter->set_result(std::move(msg));
    auto waiting_coroutine = awaiter->waiting_coroutine();
    waiting_coroutine.resume();
    if (iter->second.has_exception()) {
        // 记录异常
        // iter->second.handle.promise().exception();
    }
    if (!iter->second.coroutine.done()) {
        // 协程仍未完成，即内部再次调用了Recv等待了一个新的会话，需要重新放入等待调度队列
        RePush(id, iter->second.coroutine.promise().awaiter()->waiting_session());
    }
    else {
        tasks_.erase(iter);
    }
    return std::nullopt;
}

void MsgExecutor::AddTask(Task&& task) {
    if (task.has_exception()) {
        // 记录异常
        try {
            task.rethrow_if_exception();
        }
        catch (const std::exception& e) {
            MILLION_LOGGER_CALL(service_->service_mgr()->million(), service_->service_handle(), logger::kErr, "[million] session exception: {}.", e.what());
        }
        catch (...) {
            MILLION_LOGGER_CALL(service_->service_mgr()->million(), service_->service_handle(), logger::kErr, "[million] session exception: {}.", "unknown exception");
        }
    }
    if (!task.coroutine.done()) {
        assert(task.coroutine.promise().awaiter());
        Push(task.coroutine.promise().awaiter()->waiting_session(), std::move(task));
    }
}

void MsgExecutor::TimeoutCleanup(SessionId id) {
    auto iter = tasks_.find(id);
    if (iter == tasks_.end()) {
        return;
    }
    // 超时，写出日志告警
    MILLION_LOGGER_CALL(service_->service_mgr()->million(), service_->service_handle(), logger::kErr, "[million] session timeout {}.", iter->second.coroutine.promise().awaiter()->waiting_session());
    tasks_.erase(iter);
}

void MsgExecutor::Push(SessionId id, Task&& task) {
    service_->service_mgr()->million()->session_monitor().AddSession(service_->service_handle(), id);
    tasks_.emplace(id, std::move(task));
}

void MsgExecutor::RePush(SessionId old_id, SessionId new_id) {
    auto iter = tasks_.find(old_id);
    if (iter == tasks_.end()) {
        return;
    }
    auto task = std::move(iter->second);
    tasks_.erase(iter);
    Push(new_id, std::move(task));
}

} //namespace million