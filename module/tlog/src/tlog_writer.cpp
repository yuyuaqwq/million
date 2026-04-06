#include "million/tlog/tlog_writer.h"
#include <million/logger.h>

#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>

namespace million {
namespace tlog {

using json = nlohmann::json;

TLogWriter::TLogWriter(const TLogServiceConfig& config)
    : config_(config)
    , current_file_size_(0) {}

TLogWriter::~TLogWriter() {
    if (current_file_.is_open()) {
        current_file_.close();
    }
    if (wal_file_.is_open()) {
        wal_file_.close();
    }
}

bool TLogWriter::Init() {
    try {
        // 创建日志目录
        std::filesystem::create_directories(config_.log_dir);

        // 打开WAL文件（append模式）
        std::string wal_path = config_.log_dir + "/" + config_.wal_file;
        wal_file_.open(wal_path, std::ios::out | std::ios::app);
        if (!wal_file_.is_open()) {
            logger().LOG_ERROR("Failed to open WAL file: {}", wal_path);
            return false;
        }

        // 创建新的tlog文件
        return RotateFile();

    } catch (const std::exception& e) {
        logger().LOG_ERROR("TLogWriter init failed: {}", e.what());
        return false;
    }
}

bool TLogWriter::Write(const TLogEventData& event) {
    // 先写WAL
    std::string json_str = FormatEvent(event);
    if (!WriteWAL(json_str)) {
        logger().LOG_ERROR("Failed to write WAL for event: {}", event.event_type);
        return false;
    }

    // 检查是否需要切换文件
    if (current_file_size_ >= config_.max_file_size) {
        if (!RotateFile()) {
            logger().LOG_ERROR("Failed to rotate tlog file");
            return false;
        }
    }

    // 写入当前文件
    if (current_file_.is_open()) {
        current_file_ << json_str << std::endl;
        current_file_size_ += json_str.length() + 1; // +1 for newline

        // 如果是关键事件，立即刷新
        if (event.level == 2) { // TLOG_LEVEL_CRITICAL
            current_file_.flush();
        }

        return true;
    }

    return false;
}

bool TLogWriter::WriteBatch(const std::vector<TLogEventData>& events) {
    for (const auto& event : events) {
        if (!Write(event)) {
            return false;
        }
    }
    return true;
}

void TLogWriter::Flush() {
    if (current_file_.is_open()) {
        current_file_.flush();
    }
}

size_t TLogWriter::GetCurrentFileSize() const {
    return current_file_size_;
}

size_t TLogWriter::GetWALSize() const {
    try {
        std::string wal_path = config_.log_dir + "/" + config_.wal_file;
        return std::filesystem::file_size(wal_path);
    } catch (...) {
        return 0;
    }
}

bool TLogWriter::CheckRotate() {
    return current_file_size_ >= config_.max_file_size;
}

bool TLogWriter::RotateFile() {
    try {
        // 关闭旧文件
        if (current_file_.is_open()) {
            current_file_.close();
        }

        // 生成新文件名
        current_filename_ = GenerateFileName();
        std::string filepath = config_.log_dir + "/" + current_filename_;

        // 打开新文件
        current_file_.open(filepath, std::ios::out | std::ios::app);
        if (!current_file_.is_open()) {
            logger().LOG_ERROR("Failed to open tlog file: {}", filepath);
            return false;
        }

        current_file_size_ = 0;
        logger().LOG_INFO("Rotated to new tlog file: {}", filepath);

        return true;

    } catch (const std::exception& e) {
        logger().LOG_ERROR("Failed to rotate tlog file: {}", e.what());
        return false;
    }
}

std::string TLogWriter::FormatEvent(const TLogEventData& event) const {
    json j;
    j["event_time"] = event.event_time;
    j["event_type"] = event.event_type;
    j["player_id"] = event.player_id;
    j["server_id"] = event.server_id;
    j["event_data"] = event.event_data;
    j["device_id"] = event.device_id;
    j["ip"] = event.ip;
    j["level"] = event.level;
    j["sequence_id"] = event.sequence_id;

    return j.dump();
}

bool TLogWriter::WriteWAL(const std::string& data) {
    if (wal_file_.is_open()) {
        wal_file_ << data << std::endl;
        wal_file_.flush(); // WAL必须立即刷新
        return true;
    }
    return false;
}

std::string TLogWriter::GenerateFileName() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << "tlog_"
       << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
       << ".log";

    return ss.str();
}

} // namespace tlog
} // namespace million
