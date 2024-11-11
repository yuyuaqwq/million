#include "task_executor.h"

#include <cassert>

#include <iostream>

#include <million/imsg.h>

#include <million/logger/logger.h>

#include "service.h"
#include "service_mgr.h"
#include "session_monitor.h"
#include "million.h"

namespace million {

TaskExecutor::TaskExecutor(Service* service)
    : service_(service) {}

TaskExecutor::~TaskExecutor() = default;

// 尝试调度
std::optional<MsgUnique> TaskExecutor::TrySchedule(SessionId id, MsgUnique msg) {
    auto iter = tasks_.find(id);
    if (iter == tasks_.end()) {
        return msg;
    }
    auto awaiter = iter->second.coroutine.promise().session_awaiter();
    awaiter->set_result(std::move(msg));
    auto waiting_coroutine = awaiter->waiting_coroutine();
    waiting_coroutine.resume();
    if (iter->second.has_exception()) {
        auto million = service_->service_mgr()->million();
        // 记录异常
        try {
            iter->second.rethrow_if_exception();
        }
        catch (const std::exception& e) {
            MILLION_LOGGER_CALL(million, service_->service_handle(), logger::kErr, "[million] session exception: {}.", e.what());
        }
        catch (...) {
            MILLION_LOGGER_CALL(million, service_->service_handle(), logger::kErr, "[million] session exception: {}.", "unknown exception");
        }
    }
    if (!iter->second.coroutine.done()) {
        // 协程仍未完成，即内部再次调用了Recv等待了一个新的会话，需要重新放入等待调度队列
        RePush(id, iter->second.coroutine.promise().session_awaiter()->waiting_session());
    }
    else {
        tasks_.erase(iter);
    }
    return std::nullopt;
}

void TaskExecutor::AddTask(Task<>&& task) {
    if (task.has_exception()) {
        auto million = service_->service_mgr()->million();
        // 记录异常
        try {
            task.rethrow_if_exception();
        }
        catch (const std::exception& e) {
            MILLION_LOGGER_CALL(million, service_->service_handle(), logger::kErr, "[million] session exception: {}.", e.what());
        }
        catch (...) {
            MILLION_LOGGER_CALL(million, service_->service_handle(), logger::kErr, "[million] session exception: {}.", "unknown exception");
        }
    }
    if (!task.coroutine.done()) {
        assert(task.coroutine.promise().session_awaiter());
        Push(task.coroutine.promise().session_awaiter()->waiting_session(), std::move(task));
    }
}

void TaskExecutor::TimeoutCleanup(SessionId id) {
    auto iter = tasks_.find(id);
    if (iter == tasks_.end()) {
        return;
    }
    // 超时，写出日志告警
    auto million = service_->service_mgr()->million();
    MILLION_LOGGER_CALL(million, service_->service_handle(), logger::kErr, "[million] session timeout {}.", iter->second.coroutine.promise().session_awaiter()->waiting_session());
    tasks_.erase(iter);
}

void TaskExecutor::Push(SessionId id, Task<>&& task) {
    service_->service_mgr()->million()->session_monitor().AddSession(service_->service_handle(), id);
    tasks_.emplace(id, std::move(task));
}

void TaskExecutor::RePush(SessionId old_id, SessionId new_id) {
    auto iter = tasks_.find(old_id);
    if (iter == tasks_.end()) {
        return;
    }
    auto task = std::move(iter->second);
    tasks_.erase(iter);
    Push(new_id, std::move(task));
}

} //namespace million