#include "msg_executor.h"

#include <cassert>

#include <million/imsg.h>

namespace million {

MsgExecutor::MsgExecutor() = default;
MsgExecutor::~MsgExecutor() = default;

// 尝试调度
MsgUnique MsgExecutor::TrySchedule(SessionId id, MsgUnique msg) {
    auto iter = tasks_.find(id);
    if (iter == tasks_.end()) {
        return msg;
    }
    iter->second.handle.promise().awaiter()->set_result(std::move(msg));
    iter->second.handle.resume();
    if (iter->second.has_exception()) {
        // 记录异常
        // iter->second.handle.promise().exception();
    }
    if (!iter->second.handle.done()) {
        assert(iter->second.handle.promise().awaiter());
        // 协程仍未完成，即内部再次调用了Recv等待了一个新的会话，需要重新放入等待调度队列
        RePush(id, iter->second.handle.promise().awaiter()->get_waiting());
        return nullptr;
    }
    else {
        tasks_.erase(iter);
    }
    return nullptr;
}

void MsgExecutor::AddTask(Task&& task) {
    if (task.has_exception()) {
        // 记录异常
        // task.handle.promise().exception();
    }
    if (!task.handle.done()) {
        assert(task.handle.promise().awaiter());
        Push(task.handle.promise().awaiter()->get_waiting(), std::move(task));
    }
}

// 加入待调度队列等待调度
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