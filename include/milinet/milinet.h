#pragma once

#include <cstdint>

#include <atomic>
#include <coroutine>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <thread>
#include <condition_variable>

#include "noncopyable.h"

#include "milinet/msg.h"
#include "milinet/service.h"

namespace milinet {

// todo：分个Impl类，只暴露必要的接口

class Worker;
class Service;
class Milinet : noncopyable {
public:
    Milinet(size_t work_thread_num = kWorkThreadNum);
    ~Milinet();

    void Start();

    ServiceId AllocServiceId();
    Service& AddService(std::unique_ptr<Service> service);
    void RemoveService(ServiceId id);
    Service* GetService(ServiceId id);

    void PushService(Service* service);
    Service& PopService();

    template <typename MsgT = Msg, typename ...Args>
    std::unique_ptr<MsgT> MakeMsg(Args&&... args) {
        auto id = AllocSessionId();
        return std::make_unique<MsgT>(id, std::forward<Args>(args)...);
    }

    SessionId AllocSessionId();
    void Send(ServiceId id, MsgUnique msg);
    void Send(Service* service, MsgUnique msg);

    template <typename MsgT, typename ...Args>
    void Send(ServiceId id, Args&&... args) {
        Send(id, MakeMsg<MsgT>(std::forward<Args>(args)...));
    }

private:
    inline static size_t kWorkThreadNum = std::thread::hardware_concurrency();

    std::atomic<ServiceId> service_id_;
    std::atomic<SessionId> session_id_;

    std::vector<std::unique_ptr<Worker>> workers_;

    std::mutex service_map_mutex_;
    std::unordered_map<ServiceId, std::unique_ptr<Service>> service_map_;

    std::mutex service_queue_mutex_;
    std::queue<Service*> service_queue_;
    std::condition_variable service_queue_cv_;
};

} // namespace milinet