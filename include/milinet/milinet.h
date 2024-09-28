#pragma once

#include <cstdint>

#include <atomic>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <thread>
#include <condition_variable>

#include "noncopyable.h"

// 等待移动
#include "milinet/worker.h"
#include "milinet/service.h"

namespace milinet {

class Worker;
class Service;
class Milinet : noncopyable {
public:
    Milinet(size_t work_thread_num = kWorkThreadNum);
    ~Milinet();

    void Start();

    uint32_t AllocServiceId();
    Service& AddService(std::unique_ptr<Service> service);
    void RemoveService(uint32_t id);
    Service* GetService(uint32_t id);

    void PushService(Service* service);
    Service& PopService();

    void Send(uint32_t service_id, std::unique_ptr<Msg> msg);
    void Send(Service* service, std::unique_ptr<Msg> msg);
    // 用协程实现
    // std::unique_ptr<Msg> Call(uint32_t service_id, std::unique_ptr<Msg> msg);

private:
    inline static size_t kWorkThreadNum = std::thread::hardware_concurrency();

    std::atomic_uint32_t service_id_;

    std::vector<std::unique_ptr<Worker>> workers_;

    std::mutex service_map_mutex_;
    std::unordered_map<uint32_t, std::unique_ptr<Service>> service_map_;

    std::mutex service_queue_mutex_;
    std::queue<Service*> service_queue_;
    std::condition_variable service_queue_cv_;
};

} // namespace milinet