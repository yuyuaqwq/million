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
        {
            auto lock = std::lock_guard(adds_mutex_);
            std::swap(backup_adds_, adds_);
        }
        for (auto& task : backup_adds_) {
            tasks_.emplace(std::move(task));
        }
        backup_adds_.clear();

        auto now_time = std::chrono::high_resolution_clock::now();

        while (!tasks_.empty() && tasks_.top().expire_time <= now_time) {
            auto task = std::move(const_cast<DelayTask&>(tasks_.top()));
            imillion_->Send(task.service, task.service, std::move(task.msg));
            tasks_.pop();
        }

        std::unique_lock<std::mutex> lock(min_expire_mutex_);
        if (!tasks_.empty()) {
            min_expire_time_ = tasks_.top().expire_time;
            auto wait_duration = min_expire_time_ - now_time;
            cv_.wait_for(lock, wait_duration);
        }
        else {
            min_expire_time_ = std::chrono::high_resolution_clock::time_point::max();
            cv_.wait(lock);
        }
    }

    void AddTask(ServiceHandle service, uint32_t tick, MsgUnique msg) {
        auto expire_time = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(tick * ms_per_tick_);
        {
            auto lock = std::lock_guard(adds_mutex_);
            adds_.emplace_back(service, expire_time, std::move(msg));
        }
        bool need_notify = false;
        {
            auto lock = std::unique_lock<std::mutex>(min_expire_mutex_);
            if (expire_time < min_expire_time_) {
                min_expire_time_ = expire_time;
                need_notify = true;
            }
        }
        if (need_notify) {
            cv_.notify_one();
        }
    }

private:
    IMillion* imillion_;
    const uint32_t ms_per_tick_;

    TaskQueue tasks_;

    std::mutex adds_mutex_;
    TempTaskVector adds_;
    TempTaskVector backup_adds_;
    
    std::mutex min_expire_mutex_;
    std::chrono::high_resolution_clock::time_point min_expire_time_;
    std::condition_variable cv_;
};

} // namespace detail
} // namespace million
