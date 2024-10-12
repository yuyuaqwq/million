#pragma once

#include <cstdint>

#include <chrono>
#include <functional>
#include <array>
#include <vector>
#include <mutex>

#include "million/noncopyable.h"

#include "million/detail/functional.hpp"

namespace million {

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
        // �����ϴ�Tick���񾭹���ʱ�䣬�ƽ�
        for (size_t i = 0; i < tick_duration; i++) {
            // �õ���0����ƽ���slot��ִ������
            auto& slot = slots_[indexs_[0]];
            if (!slot.empty()) {
                for (auto& task : slot) {
                    task.func(task);
                }
                slot.clear();
            }

            // �����Ҫ���ϲ�ʱ�����ƽ������ϲ�ʱ���ֵĶ��������ɷ�
            size_t j = 0;
            // ��0���ƽ�
            ++indexs_[0];
            do {
                // ����Ѿ�����߽磬�ͻع�����ĳһ��������һȦ
                bool ge = indexs_[j] >= kSlotCount;
                if (ge) indexs_[j] = 0;
                if (j > 0) {
                    // ���ĳ��������һȦ���ͱ�ʾ�ϲ�Ҳ��Ҫ�ƽ�һ��ͬʱ���ϲ���������²��ɷ�
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
            
            // ����ʱ�������Ӧ��Ĳ���
            size_t layer = GetLayer(tick);
            size_t index = tick & kCircleMask;
            index = layer * kSlotCount + ((index + indexs_[layer]) % kSlotCount);

            // ����ڵ�ǰ���ʱ�������´������ɷ�ʱ���Բ��뵽�²�
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

    std::array<TaskVector, kSlotCount * kCircleCount> slots_;  // ���ʱ����
    std::array<uint8_t, kCircleCount> indexs_{ 0 };  // ÿһ�㵱ǰָ���slot

    std::mutex adds_mutex_;
    TaskVector adds_;
};

} // namespace detail

} // namespace million