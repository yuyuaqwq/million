#pragma once

#include <cstdint>

#include <functional>
#include <chrono>
#include <queue>
#include <vector>
#include <mutex>

namespace million {
namespace internal {

template <typename T>
class HeapTimer : noncopyable {
private:
    struct DelayTask {
        DelayTask(std::chrono::high_resolution_clock::time_point expire_time, T&& data)
            : expire_time(expire_time)
            , data(std::forward<T>(data)) {}

        bool operator>(const DelayTask& other) const {
            return expire_time > other.expire_time;
        }

        std::chrono::high_resolution_clock::time_point expire_time;
        T data;
    };

    using TaskQueue = std::priority_queue<DelayTask, std::vector<DelayTask>, std::greater<>>;
    using TempTaskVector = std::vector<DelayTask>;

public:
    HeapTimer(uint32_t ms_per_tick)
        : ms_per_tick_(ms_per_tick) {}

    ~HeapTimer() = default;

    void Init() {
        min_expire_time_ = std::chrono::high_resolution_clock::time_point::max();
    }

    void Tick(const std::function<void(DelayTask&&)>& callback) {
        {
            auto lock = std::lock_guard(mutex_);
            std::swap(backup_adds_, adds_);
        }
        for (auto& task : backup_adds_) {
            tasks_.emplace(std::move(task));
        }
        backup_adds_.clear();

        auto now_time = std::chrono::high_resolution_clock::now();

        while (!tasks_.empty() && tasks_.top().expire_time <= now_time) {
            callback(std::move(const_cast<DelayTask&>(tasks_.top())));
            tasks_.pop();
        }

        auto lock = std::unique_lock(mutex_);
        if (!tasks_.empty()) {
            cv_.wait_until(lock, tasks_.top().expire_time);
        }
        else {
            min_expire_time_ = std::chrono::high_resolution_clock::time_point::max();
            // 任务队列为空时才等待，避免唤醒丢失
            cv_.wait(lock, [this] { return !adds_.empty(); });
        }
    }

    void AddTask(uint32_t tick, T&& data) {
        auto expire_time = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(tick * ms_per_tick_);
        bool need_notify = false;
        {
            auto lock = std::unique_lock(mutex_);
            adds_.emplace_back(expire_time, std::move(data));
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
    const uint32_t ms_per_tick_;

    TaskQueue tasks_;

    std::mutex mutex_;
    TempTaskVector adds_;
    TempTaskVector backup_adds_;
    
    std::chrono::high_resolution_clock::time_point min_expire_time_;
    std::condition_variable cv_;
};

} // namespace internal
} // namespace million
