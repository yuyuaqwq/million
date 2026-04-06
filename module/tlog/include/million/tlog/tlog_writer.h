#pragma once

#include "tlog_service.h"
#include <string>
#include <fstream>
#include <memory>

namespace million {
namespace tlog {

// tlog文件写入器
class TLogWriter {
public:
    explicit TLogWriter(const TLogServiceConfig& config);
    ~TLogWriter();

    // 初始化写入器
    bool Init();

    // 写入单个事件
    bool Write(const TLogEventData& event);

    // 批量写入事件
    bool WriteBatch(const std::vector<TLogEventData>& events);

    // 强制刷新到磁盘
    void Flush();

    // 获取当前文件大小
    size_t GetCurrentFileSize() const;

    // 获取WAL文件大小
    size_t GetWALSize() const;

private:
    // 检查是否需要切换文件
    bool CheckRotate();

    // 切换到新文件
    bool RotateFile();

    // 格式化事件为JSON字符串
    std::string FormatEvent(const TLogEventData& event) const;

    // 写入WAL
    bool WriteWAL(const std::string& data);

    // 生成文件名（带时间戳）
    std::string GenerateFileName() const;

private:
    TLogServiceConfig config_;
    std::ofstream current_file_;
    std::ofstream wal_file_;
    std::string current_filename_;
    size_t current_file_size_;
};

} // namespace tlog
} // namespace million
