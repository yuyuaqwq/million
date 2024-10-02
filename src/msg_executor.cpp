#include "milinet/msg_executor.h"

#include <cassert>

#include "milinet/msg.h"

namespace milinet {

MsgExecutor::MsgExecutor() = default;
MsgExecutor::~MsgExecutor() = default;

// 尝试调度
MsgUnique MsgExecutor::TrySchedule(SessionId id, MsgUnique msg) {
    auto iter = executable_map_.find(id);
    if (iter == executable_map_.end()) {
        return msg;
    }
    iter->second.handle.promise().set_result(std::move(msg));
    iter->second.handle.resume();
    if (!iter->second.handle.done()) {
        // 协程仍未完成，即内部再次调用了Recv等待了一个新的会话，需要重新放入等待调度队列
        RePush(id, iter->second.handle.promise().get_waiting());
    }
    return nullptr;
}

// 加入待调度队列等待调度
void MsgExecutor::Push(SessionId id, Task&& task) {
    executable_map_.emplace(std::make_pair(id, std::move(task)));
}

void MsgExecutor::RePush(SessionId old_id, SessionId new_id) {
    auto iter = executable_map_.find(old_id);
    if (iter == executable_map_.end()) {
        return;
    }
    auto task = std::move(iter->second);
    executable_map_.erase(iter);
    executable_map_.emplace(std::make_pair(new_id, std::move(task)));
}

} //namespace milinet