#include "task_executor.h"

#include <cassert>

#include <iostream>

#include <million/imsg.h>
#include <million/logger.h>
#include <million/exception.h>

#include "service.h"
#include "service_mgr.h"
#include "session_monitor.h"
#include "million.h"

namespace million {

TaskExecutor::TaskExecutor(Service* service)
    : service_(service) {}

TaskExecutor::~TaskExecutor() = default;

// 尝试调度
std::optional<MsgUnique> TaskExecutor::TrySchedule(MsgUnique msg) {
    auto id = msg->session_id();
    auto iter = tasks_.find(id);
    if (iter == tasks_.end()) {
        return msg;
    }
    auto msg_opt = TrySchedule(iter->second, std::move(msg));
    if (msg_opt) {
        // find找到的task，却未处理，异常情况
        auto million = service_->service_mgr()->million();
        million->logger().Critical("[million] try schedule exception: {}.", id);
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

std::optional<MsgUnique> TaskExecutor::TrySchedule(Task<>& task, MsgUnique msg) {
    auto awaiter = task.coroutine.promise().session_awaiter();
    if (msg) {
        auto id = msg->session_id();
        if (awaiter->waiting_session() != id) {
            return msg;
        }
    }
    awaiter->set_result(std::move(msg));
    auto waiting_coroutine = awaiter->waiting_coroutine();
    waiting_coroutine.resume();
    if (task.has_exception()) {
        auto million = service_->service_mgr()->million();
        // 记录异常
        try {
            task.rethrow_if_exception();
        }
#ifdef MILLION_STACK_TRACE
        catch (const TaskAbortException& e) {
            million->logger().Err("Session exception: {}\n[Stack trace]\n{}", e.what(), e.stacktrace());
        }
#endif
        catch (const std::exception& e) {
            million->logger().Err("Session exception: {}", e.what());
        }
        catch (...) {
            million->logger().Err("Session exception: {}", "unknown exception");
        }
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
            million->logger().Err("Session exception: {}.", e.what());
        }
        catch (...) {
            million->logger().Err("Session exception: {}.", "unknown exception");
        }
    }
    if (!task.coroutine.done()) {
        assert(task.coroutine.promise().session_awaiter());
        Push(task.coroutine.promise().session_awaiter()->waiting_session(), std::move(task));
    }
}

void TaskExecutor::TaskTimeout(SessionId id) {
    auto iter = tasks_.find(id);
    if (iter == tasks_.end()) {
        // 正常情况是已经执行完毕了
        return;
    }

    // 超时，唤醒目标协程
    auto million = service_->service_mgr()->million();

    TrySchedule(iter->second, nullptr);

    if (!iter->second.coroutine.done()) {
        // 协程仍未完成，即内部再次调用了Recv等待了一个新的会话，需要重新放入等待调度队列
        RePush(id, iter->second.coroutine.promise().session_awaiter()->waiting_session());
    }
    else {
        tasks_.erase(iter);
    }
}

void TaskExecutor::Push(SessionId id, Task<>&& task) {
    service_->service_mgr()->million()->session_monitor().AddSession(service_->service_handle(), id, task.coroutine.promise().session_awaiter()->timeout_s());
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