#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>

#include "milinet/detail/noncopyable.h"
#include "milinet/msg_def.h"
#include "milinet/task.hpp"

namespace milinet {

class MsgExecutor {
public:
    MsgExecutor();
    ~MsgExecutor();

    // 尝试调度
    MsgUnique TrySchedule(SessionId id, MsgUnique msg);

    // 加入待调度队列等待调度
    void Push(SessionId id, Task&& task);

private:
    // 更新需要等待的session_id
    void RePush(SessionId old_id, SessionId new_id);

private:
    std::unordered_map<SessionId, Task> executable_map_;
};

} // namespace milinet