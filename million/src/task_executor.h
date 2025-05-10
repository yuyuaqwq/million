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
#include <million/service_handle.h>
#include <million/task.h>

namespace million {

struct TaskElement {
    TaskElement(ServiceShared sender, SessionId session_id, Task<MessagePointer> task)
        : sender(std::move(sender))
        , session_id(session_id)
        , task(std::move(task)) { }

    ServiceShared sender;
    SessionId session_id;
    Task<MessagePointer> task;
};

class ServiceCore;
class TaskExecutor {
public:
    TaskExecutor(ServiceCore* service);
    ~TaskExecutor();

    // 尝试调度
    std::variant<MessagePointer, TaskElement, TaskElement*> TrySchedule(SessionId session_id, MessagePointer msg);

    // 将任务添加到调度器
    std::optional<TaskElement> AddTask(TaskElement&& ele);

    bool TaskQueueIsEmpty() const;

    std::pair<TaskElement*, bool> TaskTimeout(SessionId id);

private:
    // 尝试调度指定Task
    std::optional<MessagePointer> TrySchedule(TaskElement& ele, SessionId session_id, MessagePointer msg);

    // 加入待调度队列等待调度
    TaskElement* Push(SessionId id, TaskElement&& ele);

    // 更新需要等待的session_id
    TaskElement* RePush(SessionId old_id, SessionId new_id);

private:
    ServiceCore* service_;
    std::unordered_map<SessionId, TaskElement> tasks_;
};

} // namespace million