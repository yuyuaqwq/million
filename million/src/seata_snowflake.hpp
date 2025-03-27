#pragma once

#include <atomic>
#include <chrono>
#include <stdexcept>

#include <million/noncopyable.h>

namespace million {

// ²Î¿¼×Ô£º
// https://www.cnblogs.com/thisiswhy/p/17611163.html
// https://seata.apache.org/zh-cn/blog/seata-analysis-UUID-generator/
using SnowId = uint64_t;
using WorkerId = uint64_t;
class SeataSnowflake : public noncopyable {
public:
    SeataSnowflake(WorkerId worker_id) {
        auto max_worker_id = (1 << kWorkerBits) - 1;
        if (worker_id > max_worker_id) {
            throw std::runtime_error("worker_id > max_worker_id.");
        }

        worker_id_ = worker_id << (kTimestampBits + kSeqBits);

        auto now = std::chrono::system_clock::now();
        auto unix_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        auto timestamp = unix_timestamp_ms - kTwepoch;

        seq_ = timestamp << kSeqBits;
    }

    ~SeataSnowflake() = default;

    SnowId NextId() {
        auto next = ++seq_;
        next &= kTimeAndSeqMask;
        return worker_id_ | next;
    }

private:
    // business meaning: machine ID (0 ~ 1023)
    // actual layout in memory:
    // highest 1 bit: 0
    // middle 10 bit: workerId
    // lowest 53 bit: all 0
    WorkerId worker_id_;
    static constexpr uint64_t kWorkerBits = 10;

    // timestamp and sequence mix in one Long
    // highest 11 bit: not used
    // middle  41 bit: timestamp
    // lowest  12 bit: sequence
    std::atomic<SnowId> seq_ = 0;
    // 2020-05-03 00:00:00
    static constexpr uint64_t kTwepoch = 1588435200000ull; 
    static constexpr uint64_t kTimestampBits = 41;
    static constexpr uint64_t kSeqBits = 12;
    static constexpr uint64_t kTimeAndSeqMask = 0x1fffffffffff;
};

} // namespace million