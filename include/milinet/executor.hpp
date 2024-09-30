#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>

#include "milinet/noncopyable.h"
#include "milinet/msg_def.h"
#include "milinet/task.hpp"

namespace milinet {

class ServiceMsgExecutor {
 public:
    
    ServiceMsgExecutor() = default;
    ~ServiceMsgExecutor() = default;

    // 尝试调度
    std::optional<MsgUnique> TrySchedule(SessionId id, MsgUnique msg) {
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
        return std::nullopt;
    }

    // 加入待调度队列等待调度
    void Push(SessionId id, Task&& task) {
        executable_map_.emplace(std::make_pair(id, std::move(task)));
    }

private:
    // 更新需要等待的session_id
    void RePush(SessionId old_id, SessionId new_id) {
        auto iter = executable_map_.find(old_id);
        if (iter == executable_map_.end()) {
            return;
        }
        auto co = std::move(iter->second);
        executable_map_.erase(iter);
        executable_map_.emplace(std::make_pair(new_id, std::move(co)));
    }

private:
    std::unordered_map<SessionId, Task> executable_map_;
};

} // namespace milinet