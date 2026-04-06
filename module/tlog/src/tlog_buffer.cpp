#include "million/tlog/tlog_buffer.h"

namespace million {
namespace tlog {

TLogBuffer::TLogBuffer(size_t max_size)
    : max_size_(max_size) {}

void TLogBuffer::Push(const TLogEventData& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(event);
}

std::vector<TLogEventData> TLogBuffer::PopAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<TLogEventData> events;
    events.reserve(queue_.size());

    while (!queue_.empty()) {
        events.push_back(queue_.front());
        queue_.pop();
    }

    return events;
}

size_t TLogBuffer::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

bool TLogBuffer::Empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

void TLogBuffer::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::queue<TLogEventData> empty;
    queue_.swap(empty);
}

} // namespace tlog
} // namespace million
