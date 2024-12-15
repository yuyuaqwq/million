#include "task_executor.h"

#include <cassert>

#include <iostream>

#include <million/proto.h>
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
std::variant<MsgUnique, Task<>*> TaskExecutor::TrySchedule(SessionId session_id, MsgUnique msg) {
    auto iter = tasks_.find(session_id);
    if (iter == tasks_.end()) {
        return msg;
    }
    auto msg_opt = TrySchedule(iter->second, session_id, std::move(msg));
    if (msg_opt) {
        // find找到的task，却未处理，异常情况
        auto& million = service_->service_mgr()->million();
        million.logger().Critical("[million] try schedule exception: {}.", session_id);
    }
    if (!iter->second.coroutine.done()) {
        // 协程仍未完成，即内部再次调用了Recv等待了一个新的会话，需要重新放入等待调度队列
        return RePush(session_id, iter->second.coroutine.promise().session_awaiter()->waiting_session());;
    }
    else {
        tasks_.erase(iter);
        return nullptr;
    }
}

bool TaskExecutor::AddTask(Task<>&& task) {
    if (task.has_exception()) {
        auto& million = service_->service_mgr()->million();
        // 记录异常
        try {
            task.rethrow_if_exception();
        }
        catch (const std::exception& e) {
            million.logger().Err("Session exception: {}.", e.what());
        }
        catch (...) {
            million.logger().Err("Session exception: {}.", "unknown exception");
        }
    }
    if (!task.coroutine.done()) {
        assert(task.coroutine.promise().session_awaiter());
        Push(task.coroutine.promise().session_awaiter()->waiting_session(), std::move(task));
        return true;
    }
    return false;
}

Task<>* TaskExecutor::TaskTimeout(SessionId session_id) {
    auto iter = tasks_.find(session_id);
    if (iter == tasks_.end()) {
        // 正常情况是已经执行完毕了
        return nullptr;
    }

    // 超时，唤醒目标协程
    TrySchedule(iter->second, session_id, nullptr);

    if (!iter->second.coroutine.done()) {
        // 协程仍未完成，即内部再次调用了Recv等待了一个新的会话，需要重新放入等待调度队列
        return RePush(session_id, iter->second.coroutine.promise().session_awaiter()->waiting_session());
    }
    else {
        tasks_.erase(iter);
        return nullptr;
    }
}

std::optional<MsgUnique> TaskExecutor::TrySchedule(Task<>& task, SessionId session_id, MsgUnique msg) {
    auto awaiter = task.coroutine.promise().session_awaiter();
    if (msg) {
        if (awaiter->waiting_session() != session_id) {
            return msg;
        }
    }
    awaiter->set_result(std::move(msg));
    auto waiting_coroutine = awaiter->waiting_coroutine();
    waiting_coroutine.resume();
    if (task.has_exception()) {
        auto& million = service_->service_mgr()->million();
        // 记录异常
        try {
            task.rethrow_if_exception();
        }
#ifdef MILLION_STACK_TRACE
        catch (const TaskAbortException& e) {
            million.logger().Err("Session exception: {}\n[Stack trace]\n{}", e.what(), e.stacktrace());
        }
#endif
        catch (const std::exception& e) {
            million.logger().Err("Session exception: {}", e.what());
        }
        catch (...) {
            million.logger().Err("Session exception: {}", "unknown exception");
        }
    }
    return std::nullopt;
}

Task<>* TaskExecutor::Push(SessionId id, Task<>&& task) {
    auto& million = service_->service_mgr()->million();
    if (id == kSessionIdInvalid) {
        million.logger().Err("Waiting for an invalid session.");
        return nullptr;
    }
    million.session_monitor().AddSession(service_->service_handle(), id, task.coroutine.promise().session_awaiter()->timeout_s());
    auto res = tasks_.emplace(id, std::move(task));
    if (!res.second) {
        // throw std::runtime_error("Duplicate session id.");
        million.logger().Err("Found duplicate session id: {}", id);
        return nullptr;
    }
    return &res.first->second;
}

Task<>* TaskExecutor::RePush(SessionId old_id, SessionId new_id) {
    auto iter = tasks_.find(old_id);
    if (iter == tasks_.end()) {
        return nullptr;
    }
    auto task = std::move(iter->second);
    tasks_.erase(iter);
    return Push(new_id, std::move(task));
}

} //namespace million