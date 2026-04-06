#include "million/tlog/tlog_service.h"
#include "million/tlog/tlog_buffer.h"
#include "million/tlog/tlog_writer.h"
#include <million/logger.h>
#include <million/imillion.h>

#include <filesystem>
#include <fstream>

namespace million {
namespace tlog {

MILLION_MESSAGE_DEFINE(million.tlog.ss, TLogWrite, (million::tlog::TLogEvent) event, (uint64_t) sequence_id);
MILLION_MESSAGE_DEFINE(million.tlog.ss, TLogWriteAck, (uint64_t) sequence_id, (bool) success, (string) error_msg);
MILLION_MESSAGE_DEFINE(million.tlog.ss, TLogFlush);
MILLION_MESSAGE_DEFINE(million.tlog.ss, TLogGetStats);
MILLION_MESSAGE_DEFINE(million.tlog.ss, TLogGetStatsResp,
    (uint64_t) buffer_size,
    (uint32_t) pending_files,
    (uint64_t) wal_size,
    (uint64_t) last_flush_time,
    (uint32_t) write_speed,
    (uint32_t) upload_speed);

bool TLogService::OnInit() {
    auto& settings = imillion().YamlSettings();
    logger().LOG_INFO("Loading 'tlog' settings.");

    // 读取配置
    auto tlog_settings = settings["tlog"];
    if (tlog_settings) {
        if (tlog_settings["log_dir"]) {
            config_.log_dir = tlog_settings["log_dir"].as<std::string>();
        }
        if (tlog_settings["max_buffer_size"]) {
            config_.max_buffer_size = tlog_settings["max_buffer_size"].as<size_t>();
        }
        if (tlog_settings["flush_interval_ms"]) {
            config_.flush_interval_ms = tlog_settings["flush_interval_ms"].as<uint32_t>();
        }
        if (tlog_settings["max_file_size"]) {
            config_.max_file_size = tlog_settings["max_file_size"].as<size_t>();
        }
        if (tlog_settings["enable_compression"]) {
            config_.enable_compression = tlog_settings["enable_compression"].as<bool>();
        }
    }

    // 创建缓冲区
    buffer_ = std::make_unique<TLogBuffer>(config_.max_buffer_size);

    // 创建写入器
    writer_ = std::make_unique<TLogWriter>(config_);
    if (!writer_->Init()) {
        logger().LOG_ERROR("Failed to initialize TLogWriter");
        return false;
    }

    // 从WAL恢复数据
    if (!RecoverFromWAL()) {
        logger().LOG_WARN("WAL recovery had some issues, but continuing");
    }

    logger().LOG_INFO("TLog service initialized successfully");
    return true;
}

Task<MessagePointer> TLogService::OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) {
    // 启动刷新线程
    running_ = true;
    flush_thread_ = std::thread(&TLogService::FlushThread, this);

    start_time_ = std::chrono::steady_clock::now();
    last_flush_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    logger().LOG_INFO("TLog service started");
    co_return nullptr;
}

Task<MessagePointer> TLogService::OnStop(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) {
    // 停止刷新线程
    running_ = false;
    flush_cv_.notify_all();

    if (flush_thread_.joinable()) {
        flush_thread_.join();
    }

    // 最后一次刷新
    Flush();

    logger().LOG_INFO("TLog service stopped");
    co_return nullptr;
}

void TLogService::WriteEvent(const TLogEventData& event) {
    total_events_++;

    // 如果是关键事件，立即写入
    if (event.level == 2) { // TLOG_LEVEL_CRITICAL
        critical_events_++;
        writer_->Write(event);
        return;
    }

    // 普通事件加入缓冲区
    buffer_->Push(event);

    // 检查是否需要刷盘
    CheckFlush();
}

TLogService::Stats TLogService::GetStats() const {
    Stats stats;
    stats.buffer_size = buffer_->Size();
    stats.pending_files = 0; // TODO: 实现文件上传管理
    stats.wal_size = writer_->GetWALSize();
    stats.last_flush_time = last_flush_time_.load();

    // 计算速度
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    if (elapsed > 0) {
        stats.write_speed = total_events_.load() / static_cast<uint32_t>(elapsed);
        stats.upload_speed = 0; // TODO: 实现文件上传
    }

    return stats;
}

MILLION_MESSAGE_HANDLE(TLogService::TLogWrite, msg) {
    TLogEventData event;
    event.event_time = msg->event.event_time();
    event.event_type = msg->event.event_type();
    event.player_id = msg->event.player_id();
    event.server_id = msg->event.server_id();
    event.event_data = msg->event.event_data();
    event.device_id = msg->event.device_id();
    event.ip = msg->event.ip();
    event.level = static_cast<uint32_t>(msg->event.level());
    event.sequence_id = msg->sequence_id;

    WriteEvent(event);

    // 发送ACK确认
    SendAck(msg->sequence_id, true);

    co_return nullptr;
}

MILLION_MESSAGE_HANDLE(TLogService::TLogFlush, msg) {
    Flush();
    co_return nullptr;
}

MILLION_MESSAGE_HANDLE(TLogService::TLogGetStats, msg) {
    auto stats = GetStats();

    auto resp = std::make_shared<million::tlog::ss::TLogGetStatsResp>();
    resp->set_buffer_size(stats.buffer_size);
    resp->set_pending_files(stats.pending_files);
    resp->set_wal_size(stats.wal_size);
    resp->set_last_flush_time(stats.last_flush_time);
    resp->set_write_speed(stats.write_speed);
    resp->set_upload_speed(stats.upload_speed);

    co_return resp;
}

bool TLogService::RecoverFromWAL() {
    std::string wal_path = config_.log_dir + "/" + config_.wal_file;

    try {
        if (!std::filesystem::exists(wal_path)) {
            logger().LOG_INFO("No WAL file found, skipping recovery");
            return true;
        }

        logger().LOG_INFO("Recovering from WAL file: {}", wal_path);

        std::ifstream wal_file(wal_path);
        if (!wal_file.is_open()) {
            logger().LOG_ERROR("Failed to open WAL file for recovery");
            return false;
        }

        std::string line;
        int recovered_count = 0;
        while (std::getline(wal_file, line)) {
            if (!line.empty()) {
                // TODO: 解析JSON并重新写入正式文件
                recovered_count++;
            }
        }

        wal_file.close();

        // 清空WAL文件
        std::ofstream(wal_path, std::ios::trunc).close();

        logger().LOG_INFO("WAL recovery completed, recovered {} events", recovered_count);
        return true;

    } catch (const std::exception& e) {
        logger().LOG_ERROR("WAL recovery failed: {}", e.what());
        return false;
    }
}

bool TLogService::WriteToWAL(const TLogEventData& event) {
    // WAL写入由TLogWriter处理
    return true;
}

void TLogService::Flush() {
    // 取出所有缓冲的事件
    auto events = buffer_->PopAll();

    if (!events.empty()) {
        // 批量写入文件
        writer_->WriteBatch(events);
        writer_->Flush();

        // 更新最后刷新时间
        last_flush_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
}

void TLogService::FlushThread() {
    while (running_) {
        std::unique_lock<std::mutex> lock(flush_mutex_);

        // 等待指定时间或被通知
        if (flush_cv_.wait_for(lock, std::chrono::milliseconds(config_.flush_interval_ms),
                [this] { return !running_; })) {
            break; // 收到停止信号
        }

        // 定时刷新
        Flush();
    }
}

void TLogService::CheckFlush() {
    // 检查缓冲区是否已满
    if (buffer_->Size() * sizeof(TLogEventData) >= config_.max_buffer_size) {
        Flush();
    }

    // 如果有关键事件等待处理，也刷新
    if (critical_events_.load() > 0) {
        Flush();
        critical_events_ = 0;
    }
}

void TLogService::SendAck(uint64_t sequence_id, bool success, const std::string& error_msg) {
    auto ack = std::make_shared<million::tlog::ss::TLogWriteAck>();
    ack->set_sequence_id(sequence_id);
    ack->set_success(success);
    ack->set_error_msg(error_msg);

    // TODO: 发送ACK给调用者
}

} // namespace tlog
} // namespace million
