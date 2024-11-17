#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>

#include <million/noncopyable.h>
#include <million/msg_def.h>
#include <million/task.h>

namespace million {

class Service;
class TaskExecutor {
public:
    TaskExecutor(Service* service);
    ~TaskExecutor();

    // 尝试调度
    std::optional<MsgUnique> TrySchedule(MsgUnique msg);
    std::optional<MsgUnique> TrySchedule(Task<>& task, MsgUnique msg);

    // 将任务添加到调度器
    void AddTask(Task<>&& task);

    std::optional<Task<>> TimeoutCleanup(SessionId id);

private:
    // 加入待调度队列等待调度
    void Push(SessionId id, Task<>&& task);

    // 更新需要等待的session_id
    void RePush(SessionId old_id, SessionId new_id);

private:
    Service* service_;
    std::unordered_map<SessionId, Task<>> tasks_;
};

} // namespace million