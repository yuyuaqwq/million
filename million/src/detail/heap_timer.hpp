#pragma once

#include <cstdint>
#include <chrono>
#include <queue>
#include <vector>
#include <mutex>

#include <million/imillion.h>

namespace million {
namespace detail {

class HeapTimer : noncopyable {
private:
    struct DelayTask {
        DelayTask(ServiceHandle service, std::chrono::high_resolution_clock::time_point expire_time, MsgUnique msg)
            : expire_time(expire_time)
            , service(service)
            , msg(std::move(msg)) {}

        ServiceHandle service;
        std::chrono::high_resolution_clock::time_point expire_time;
        MsgUnique msg;

        bool operator>(const DelayTask& other) const {
            return expire_time > other.expire_time;
        }
    };

    using TaskQueue = std::priority_queue<DelayTask, std::vector<DelayTask>, std::greater<>>;
    using TempTaskVector = std::vector<DelayTask>;

public:
    HeapTimer(IMillion* imillion, uint32_t ms_per_tick)
        : imillion_(imillion)
        , ms_per_tick_(ms_per_tick) {}

    ~HeapTimer() = default;

    void Init() {
        
    }

    void Tick() {
        // 将等待队列中的任务批量加入到优先级队列
        TempTaskVector tmp_queue;
        {
            std::lock_guard guard(adds_mutex_);
            std::swap(tmp_queue, adds_);
        }
        for (auto& task : tmp_queue) {
            tasks_.push(std::move(task));
        }

        auto now_time = std::chrono::high_resolution_clock::now();

        // 执行到期的任务
        while (!tasks_.empty() && tasks_.top().expire_time <= now_time) {
            auto task = std::move(const_cast<DelayTask&>(tasks_.top()));
            imillion_->Send(task.service, task.service, std::move(task.msg));
            tasks_.pop();
        }
    }

    void AddTask(ServiceHandle service, uint32_t tick, MsgUnique msg) {
        auto expire_time = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(tick * ms_per_tick_);

        auto guard = std::lock_guard(adds_mutex_);
        adds_.emplace_back(service, expire_time, std::move(msg));
    }

    uint32_t ms_per_tick() const { return ms_per_tick_; }

private:
    IMillion* imillion_;
    const uint32_t ms_per_tick_;

    TaskQueue tasks_;

    std::mutex adds_mutex_;
    TempTaskVector adds_;
    
};

} // namespace detail
} // namespace million
