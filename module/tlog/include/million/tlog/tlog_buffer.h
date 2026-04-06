#pragma once

#include "tlog_service.h"
#include <queue>
#include <mutex>

namespace million {
namespace tlog {

// tlog缓冲区
class TLogBuffer {
public:
    explicit TLogBuffer(size_t max_size);
    ~TLogBuffer() = default;

    // 添加事件到缓冲区
    void Push(const TLogEventData& event);

    // 批量取出事件（用于刷盘）
    std::vector<TLogEventData> PopAll();

    // 获取当前缓冲区大小
    size_t Size() const;

    // 是否为空
    bool Empty() const;

    // 清空缓冲区
    void Clear();

private:
    size_t max_size_;
    std::queue<TLogEventData> queue_;
    mutable std::mutex mutex_;
};

} // namespace tlog
} // namespace million
