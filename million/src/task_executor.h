#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <variant>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>

#include <million/noncopyable.h>
#include <million/session_def.h>
#include <million/task.h>

namespace million {

class Service;
class TaskExecutor {
public:
    TaskExecutor(Service* service);
    ~TaskExecutor();

    // 尝试调度
    std::variant<MsgUnique, Task<MsgUnique>, Task<MsgUnique>*> TrySchedule(SessionId session_id, MsgUnique msg);

    // 将任务添加到调度器
    std::optional<Task<MsgUnique>> AddTask(Task<MsgUnique>&& task);

    bool TaskQueueIsEmpty() const;

    std::pair<Task<MsgUnique>*, bool> TaskTimeout(SessionId id);

private:
    // 尝试调度指定Task
    std::optional<MsgUnique> TrySchedule(Task<MsgUnique>& task, SessionId session_id, MsgUnique msg);

    // 加入待调度队列等待调度
    Task<MsgUnique>* Push(SessionId id, Task<MsgUnique>&& task);

    // 更新需要等待的session_id
    Task<MsgUnique>* RePush(SessionId old_id, SessionId new_id);

private:
    Service* service_;
    std::unordered_map<SessionId, Task<MsgUnique>> tasks_;
};

} // namespace million