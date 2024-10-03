#pragma once

#include <cstdint>

#include <chrono>
#include <functional>
#include <array>
#include <vector>
#include <mutex>

#include "milinet/noncopyable.h"

namespace milinet {

namespace detail {

class TimeWheel : noncopyable {
public:
    struct Task {
        using Func = std::function<void(const Task&)>;

        Task(size_t tick, const Func& func)
            : tick(tick)
            , func(func) {}
        ~Task() = default;

        Task(const Task&) = default;

        size_t tick;
        Func func;
    };

    using TaskVector = std::vector<Task>;

public:
    TimeWheel(size_t one_tick_ms)
        : one_tick_ms_(one_tick_ms) {}
    ~TimeWheel() = default;

    void Init() {
        last_time_ = std::chrono::high_resolution_clock::now();
    }

    void Tick() {
        TaskVector tmp_queue;
        {
            std::lock_guard guard(adds_mutex_);
            std::swap(tmp_queue, adds_);
        }
        if (!tmp_queue.empty()) {
            DispatchTasks(&tmp_queue);
        }

        auto now_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - last_time_);
        size_t tick_duration = duration.count() / one_tick_ms_;
        // 根据上次Tick至今经过的时间，推进
        for (size_t i = 0; i < tick_duration; i++) {
            // 拿到第0层待推进的slot，执行任务
            auto& slot = slots_[indexs_[0]];
            if (!slot.empty()) {
                for (auto& task : slot) {
                    task.func(task);
                }
                slot.clear();
            }

            // 如果需要向上层时间轮推进，则将上层时间轮的队列向下派发
            size_t j = 0;
            // 第0层推进
            ++indexs_[0];
            do {
                // 如果已经到达边界，就回滚，即某一层走完了一圈
                bool ge = indexs_[j] >= kSlotCount;
                if (ge) indexs_[j] = 0;
                if (j > 0) {
                    // 如果某层走完了一圈，就表示上层也需要推进一格，同时将上层的任务向下层派发
                    size_t index = j * kSlotCount + indexs_[j];
                    DispatchTasks(&slots_[index]);
                }
                if (!ge || ++j >= kCircleCount) break;
                ++indexs_[j];
            } while (true);
        }
        last_time_ += std::chrono::milliseconds(tick_duration * one_tick_ms_);
    }

    void AddTaskWithGuard(size_t tick, const Task::Func& func) {
        adds_.emplace_back(tick, func);
    }

    void AddTask(size_t tick, const Task::Func& func) {
        {
            std::lock_guard guard(adds_mutex_);
            AddTaskWithGuard(tick, func);
        }
    }

private:
    size_t GetLayer(size_t tick) {
        for (size_t i = 0; i < kCircleCount; i++) {
            if ((tick & ~kCircleMask) == 0) {
                return i;
            }
            tick >>= 6;
        }
        throw std::invalid_argument("tick too large.");
    }

    void DispatchTasks(TaskVector* tasks) {
        for (auto& task : *tasks) {
            size_t tick = task.tick;
            
            // 根据时长插入对应层的槽中
            size_t layer = GetLayer(tick);
            size_t index = tick & kCircleMask;
            index = layer * kSlotCount + ((index + indexs_[layer]) % kSlotCount);

            // 清除在当前层的时长，在下次向下派发时可以插入到下层
            size_t mask = ~(std::numeric_limits<size_t>::max() << (layer * kSlotBit));
            task.tick &= mask;

            slots_[index].emplace_back(task);
        }
        tasks->clear();
    }

private:
    static constexpr size_t kCircleCount = 5;
    static constexpr size_t kSlotBit = 6;
    static constexpr size_t kSlotCount = 1 << kSlotBit; // 64
    static constexpr size_t kCircleMask = 0x3f; // 0011 1111

    size_t one_tick_ms_;

    std::chrono::high_resolution_clock::time_point last_time_;

    std::array<TaskVector, kSlotCount * kCircleCount> slots_;  // 多层时间轮
    std::array<uint8_t, kCircleCount> indexs_{ 0 };  // 每一层当前指向的slot

    std::mutex adds_mutex_;
    TaskVector adds_;
};

} // namespace detail

} // namespace milinet