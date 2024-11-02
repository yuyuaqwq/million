#include "msg_executor.h"

#include <cassert>

#include <million/imsg.h>

namespace million {

MsgExecutor::MsgExecutor() = default;
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
    //if (task.has_exception()) {
    //    // 记录异常
    //    // task.coroutine.promise().exception();
    //}
    if (!task.coroutine.done()) {
        assert(task.coroutine.promise().awaiter());
        Push(task.coroutine.promise().awaiter()->waiting_session(), std::move(task));
    }
}

void MsgExecutor::Push(SessionId id, Task&& task) {
    tasks_.emplace(std::make_pair(id, std::move(task)));
}

void MsgExecutor::RePush(SessionId old_id, SessionId new_id) {
    auto iter = tasks_.find(old_id);
    if (iter == tasks_.end()) {
        return;
    }
    auto task = std::move(iter->second);
    tasks_.erase(iter);
    tasks_.emplace(std::make_pair(new_id, std::move(task)));
}

} //namespace million