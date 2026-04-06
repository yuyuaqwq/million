#pragma once

#include <million/iservice.h>
#include <million/tlog/api.h>

#include <queue>
#include <mutex>
#include <fstream>
#include <atomic>
#include <chrono>
#include <condition_variable>

namespace million {
namespace tlog {

// tlog服务配置
struct TLogServiceConfig {
    std::string log_dir = "tlog";              // tlog日志目录
    std::string wal_file = "tlog.wal";         // WAL文件名
    size_t max_buffer_size = 10 * 1024 * 1024; // 最大缓冲区大小（10MB）
    uint32_t flush_interval_ms = 5000;         // 刷盘间隔（5秒）
    size_t max_file_size = 100 * 1024 * 1024;  // 单个文件最大大小（100MB）
    bool enable_compression = false;           // 是否启用压缩
};

// tlog事件
struct TLogEventData {
    uint64_t event_time;
    std::string event_type;
    uint64_t player_id;
    uint32_t server_id;
    std::string event_data;
    std::string device_id;
    std::string ip;
    uint32_t level; // 0: stat, 1: info, 2: critical
    uint64_t sequence_id; // 用于ACK确认
};

class TLogBuffer;
class TLogWriter;

class TLogService : public IService {
    MILLION_SERVICE_DEFINE(TLogService);

public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override;
    virtual Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override;
    virtual Task<MessagePointer> OnStop(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override;

    // 写入tlog事件
    void WriteEvent(const TLogEventData& event);

    // 获取统计信息
    struct Stats {
        size_t buffer_size;
        uint32_t pending_files;
        size_t wal_size;
        uint64_t last_flush_time;
        uint32_t write_speed;
        uint32_t upload_speed;
    };
    Stats GetStats() const;

    MILLION_MESSAGE_HANDLE(TLogWrite, msg);
    MILLION_MESSAGE_HANDLE(TLogFlush, msg);
    MILLION_MESSAGE_HANDLE(TLogGetStats, msg);

private:
    // 从WAL恢复数据
    bool RecoverFromWAL();

    // 写入WAL
    bool WriteToWAL(const TLogEventData& event);

    // 刷盘到正式文件
    void Flush();

    // 后台刷新线程
    void FlushThread();

    // 检查是否需要刷盘
    void CheckFlush();

    // 发送ACK确认
    void SendAck(uint64_t sequence_id, bool success, const std::string& error_msg = "");

    // 生成新的序列ID
    uint64_t NextSequenceId() { return ++sequence_id_; }

private:
    TLogServiceConfig config_;
    std::unique_ptr<TLogBuffer> buffer_;
    std::unique_ptr<TLogWriter> writer_;

    std::atomic<uint64_t> sequence_id_{0};
    std::atomic<uint64_t> last_flush_time_{0};

    std::thread flush_thread_;
    std::atomic<bool> running_{false};
    std::condition_variable flush_cv_;
    std::mutex flush_mutex_;

    // 统计信息
    std::atomic<uint32_t> total_events_{0};
    std::atomic<uint32_t> critical_events_{0};
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace tlog
} // namespace million
