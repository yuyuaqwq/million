#pragma once

#include <cstdint>

#include <functional>
#include <chrono>
#include <array>
#include <list>
#include <vector>
#include <mutex>

namespace million {
namespace internal {

template <typename T>
class WheelTimer : noncopyable {
protected:
    struct DelayTask {
        DelayTask(uint32_t tick, T&& data)
            : tick(tick)
            , data(std::forward<T>(data)) {}

        uint32_t tick;
        T data;
    };
    using TaskQueue = std::vector<DelayTask>;

public:
    WheelTimer(uint32_t ms_per_tick)
        : ms_per_tick_(ms_per_tick) {}
    ~WheelTimer() = default;

    void Init() {
        last_time_ = std::chrono::high_resolution_clock::now();
    }

    void Tick(const std::function<void(DelayTask&&)>& callback) {
        {
            auto lock = std::lock_guard(adds_mutex_);
            std::swap(backup_adds_, adds_);
        }
        if (!backup_adds_.empty()) {
            DispatchTasks(&backup_adds_);
        }
       
        auto now_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - last_time_);
        size_t tick_duration = duration.count() / ms_per_tick_;
        // 根据上次Tick至今经过的时间，推进
        for (size_t i = 0; i < tick_duration; i++) {
            // 拿到第0层待推进的slot，执行任务
            auto& slot = slots_[indexs_[0]];
            if (!slot.empty()) {
                for (auto&& task : slot) {
                    callback(std::move(task));
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
        last_time_ += std::chrono::milliseconds(tick_duration * ms_per_tick_);

        std::this_thread::sleep_for(std::chrono::milliseconds(ms_per_tick_));
    }

    void AddTask(uint32_t tick, T&& data) {
        auto lock = std::lock_guard(adds_mutex_);
        adds_.emplace_back(tick, std::move(data));
    }

protected:
    // std::pair<layer, index>
    std::pair<size_t, size_t> GetLayer(uint32_t tick) {
        for (size_t i = 0; i < kCircleCount; i++) {
            if ((tick & ~kCircleMask) == 0) {
                return { i, tick };
            }
            tick >>= kSlotBit;
        }
        throw std::invalid_argument("tick too large.");
    }

    void DispatchTasks(TaskQueue* tasks) {
        for (auto& task : *tasks) {
            size_t tick = task.tick;
            
            // 根据时长插入对应层的槽中
            auto&& [layer, index] = GetLayer(tick);
            index = layer * kSlotCount + ((index + indexs_[layer]) % kSlotCount);

            // 清除在当前层的时长，在下次向下派发时可以插入到下层
            size_t mask = ~(std::numeric_limits<size_t>::max() << (layer * kSlotBit));
            task.tick &= mask;

            slots_[index].emplace_back(std::move(task));
        }
        tasks->clear();
    }

protected:
    static constexpr size_t kCircleCount = 5;
    static constexpr size_t kSlotBit = 6;
    static constexpr size_t kSlotCount = 1 << kSlotBit; // 64
    static constexpr size_t kCircleMask = 0x3f; // 0011 1111

    const uint32_t ms_per_tick_;

    std::chrono::high_resolution_clock::time_point last_time_;

    std::array<TaskQueue, kSlotCount * kCircleCount> slots_;  // 多层时间轮
    std::array<uint16_t, kCircleCount> indexs_{ 0 };  // 每一层当前指向的slot

    std::mutex adds_mutex_;
    TaskQueue adds_;
    TaskQueue backup_adds_;
};

} // namespace internal
} // namespace million