#include "task_executor.h"

#include <cassert>

#include <iostream>

#include <million/message.h>
#include <million/logger.h>
#include <million/exception.h>

#include "service_core.h"
#include "service_mgr.h"
#include "session_monitor.h"
#include "million.h"

namespace million {

TaskExecutor::TaskExecutor(ServiceCore* service)
    : service_(service) {}

TaskExecutor::~TaskExecutor() = default;

// 尝试调度
std::variant<MessagePointer, TaskElement, TaskElement*> TaskExecutor::TrySchedule(SessionId session_id, MessagePointer msg) {
    auto iter = tasks_.find(session_id);
    if (iter == tasks_.end()) {
        return msg;
    }
    auto msg_opt = TrySchedule(iter->second, session_id, std::move(msg));
    if (msg_opt) {
        // find找到的task，却未处理，异常情况
        auto& million = service_->service_mgr()->million();
        million.logger().LOG_CRITICAL("Try schedule exception: {}.", session_id);
    }
    if (!iter->second.task.coroutine.done()) {
        // 协程仍未完成，即内部再次调用了Recv等待了一个新的会话，需要重新放入等待调度队列
        auto task = RePush(session_id, iter->second.task.coroutine.promise().session_awaiter()->waiting_session_id());
        return task;
    }
    else {
        auto task = std::move(iter->second);
        tasks_.erase(iter);
        return task;
    }
}

std::optional<TaskElement> TaskExecutor::AddTask(TaskElement&& ele) {
    if (ele.task.has_exception()) {
        auto& million = service_->service_mgr()->million();
        // 记录异常
        try {
            ele.task.rethrow_if_exception();
        }
        catch (const std::exception& e) {
            million.logger().LOG_ERROR("Session exception: {}", e.what());
        }
        catch (...) {
            million.logger().LOG_ERROR("Session exception: {}", "unknown exception.");
        }
    }
    if (!ele.task.coroutine.done()) {
        assert(ele.task.coroutine.promise().session_awaiter());
        Push(ele.task.coroutine.promise().session_awaiter()->waiting_session_id(), std::move(ele));
        return std::nullopt;
    }
    return ele;
}

bool TaskExecutor::TaskQueueIsEmpty() const {
    return tasks_.empty();
}

std::pair<TaskElement*, bool> TaskExecutor::TaskTimeout(SessionId session_id) {
    auto iter = tasks_.find(session_id);
    if (iter == tasks_.end()) {
        // 正常情况是已经执行完毕了
        return std::make_pair(nullptr, false);
    }

    // 超时，唤醒目标协程
    TrySchedule(iter->second, session_id, nullptr);

    if (!iter->second.task.coroutine.done()) {
        // 协程仍未完成，即内部再次调用了Recv等待了一个新的会话，需要重新放入等待调度队列
        return std::make_pair(RePush(session_id, iter->second.task.coroutine.promise().session_awaiter()->waiting_session_id()), false);
    }
    else {
        auto has_exception = iter->second.task.has_exception();
        tasks_.erase(iter);
        return std::make_pair(nullptr, has_exception);
    }
}

std::optional<MessagePointer> TaskExecutor::TrySchedule(TaskElement& ele, SessionId session_id, MessagePointer msg) {
    auto awaiter = ele.task.coroutine.promise().session_awaiter();
    if (msg) {
        if (awaiter->waiting_session_id() != session_id) {
            return msg;
        }
    }
    awaiter->set_result(std::move(msg));
    auto waiting_coroutine = awaiter->waiting_coroutine();
    waiting_coroutine.resume();
    if (ele.task.has_exception()) {
        auto& million = service_->service_mgr()->million();
        // 记录异常
        try {
            ele.task.rethrow_if_exception();
        }
#ifdef MILLION_STACK_TRACE
        catch (const TaskAbortException& e) {
            million.logger().LOG_ERROR("Session {} exception: {}\n[Stack trace]\n{}", session_id, e.what(), e.stacktrace());
        }
#endif
        catch (const std::exception& e) {
            million.logger().LOG_ERROR("Session {} exception: {}", session_id, e.what());
        }
        catch (...) {
            million.logger().LOG_ERROR("Session {} exception: {}", session_id, "unknown exception.");
        }
    }
    return std::nullopt;
}

TaskElement* TaskExecutor::Push(SessionId id, TaskElement&& ele) {
    assert(!SessionIsReplyId(id));
    auto& million = service_->service_mgr()->million();
    if (id == kSessionIdInvalid) {
        million.logger().LOG_ERROR("Waiting for an invalid session.");
        return nullptr;
    }
    auto timeout_s = ele.task.coroutine.promise().session_awaiter()->timeout_s();
    if (timeout_s != kSessionNeverTimeout) {
        million.session_monitor().AddSession(service_->shared(), id, timeout_s);
    }
    auto res = tasks_.emplace(id, std::move(ele));
    if (!res.second) {
        // throw std::runtime_error("Duplicate session id.");
        million.logger().LOG_ERROR("Found duplicate session id: {}.", id);
        return nullptr;
    }
    return &res.first->second;
}

TaskElement* TaskExecutor::RePush(SessionId old_id, SessionId new_id) {
    auto iter = tasks_.find(old_id);
    if (iter == tasks_.end()) {
        return nullptr;
    }
    auto task = std::move(iter->second);
    tasks_.erase(iter);
    return Push(new_id, std::move(task));
}

} //namespace million