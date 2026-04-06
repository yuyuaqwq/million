#include "tlog_module.h"

#include <chrono>
#include <million/jssvr/jssvr.h>
#include <million/tlog/ss_tlog.pb.h>

#include "../js_service.h"

namespace million {
namespace jssvr {

TLogModuleObject::TLogModuleObject(mjs::Runtime* rt)
    : CppModuleObject(rt)
{
    AddExportMethod(rt, "info", Info);
    AddExportMethod(rt, "critical", Critical);
    AddExportMethod(rt, "stat", Stat);
    AddExportMethod(rt, "getStats", GetStats);
}

mjs::Value TLogModuleObject::Info(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSRuntineService(&context->runtime());

    if (par_count < 1) {
        return mjs::Error::Throw(context, "tlog.info requires at least 1 parameter (event_type)");
    }

    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "tlog.info parameter 1 must be a string (event_type)");
    }

    auto event_type = stack.get(0).ToString(context).string_view();

    // 构建tlog事件数据
    auto event = std::make_shared<million::tlog::ss::TLogWrite>();
    auto* tlog_event = event->mutable_event();
    tlog_event->set_event_time(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    tlog_event->set_event_type(std::string(event_type));
    tlog_event->set_level(million::tlog::TLogLevel::TLOG_LEVEL_INFO);

    // 解析第二个参数（事件数据对象）
    if (par_count >= 2 && stack.get(1).IsObject()) {
        auto data_obj = stack.get(1);
        // 将JS对象转换为JSON字符串
        // TODO: 实现完整的JSON序列化
        tlog_event->set_event_data("{}");
    }

    // 解析选项参数
    if (par_count >= 3 && stack.get(2).IsObject()) {
        auto options_obj = stack.get(2);
        // TODO: 解析选项
    }

    event->set_sequence_id(service.imillion().NextSequenceId());

    // 发送到tlog服务
    service.imillion().Send(service.service_handle(), "tlog", "TLogWrite", event);

    return mjs::Value();
}

mjs::Value TLogModuleObject::Critical(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSRuntineService(&context->runtime());

    if (par_count < 1) {
        return mjs::Error::Throw(context, "tlog.critical requires at least 1 parameter (event_type)");
    }

    if (!stack.get(0).IsString()) {
        return mjs::Error::Throw(context, "tlog.critical parameter 1 must be a string (event_type)");
    }

    auto event_type = stack.get(0).ToString(context).string_view();

    // 构建tlog事件数据（关键级别）
    auto event = std::make_shared<million::tlog::ss::TLogWrite>();
    auto* tlog_event = event->mutable_event();
    tlog_event->set_event_time(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    tlog_event->set_event_type(std::string(event_type));
    tlog_event->set_level(million::tlog::TLogLevel::TLOG_LEVEL_CRITICAL);

    if (par_count >= 2 && stack.get(1).IsObject()) {
        tlog_event->set_event_data("{}");
    }

    event->set_sequence_id(service.imillion().NextSequenceId());

    // 发送到tlog服务
    service.imillion().Send(service.service_handle(), "tlog", "TLogWrite", event);

    return mjs::Value();
}

mjs::Value TLogModuleObject::Stat(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSRuntineService(&context->runtime());

    if (par_count < 1) {
        return mjs::Error::Throw(context, "tlog.stat requires at least 1 parameter (event_type)");
    }

    auto event_type = stack.get(0).ToString(context).string_view();

    // 构建tlog事件数据（统计级别）
    auto event = std::make_shared<million::tlog::ss::TLogWrite>();
    auto* tlog_event = event->mutable_event();
    tlog_event->set_event_time(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    tlog_event->set_event_type(std::string(event_type));
    tlog_event->set_level(million::tlog::TLogLevel::TLOG_LEVEL_STAT);

    if (par_count >= 2 && stack.get(1).IsObject()) {
        tlog_event->set_event_data("{}");
    }

    event->set_sequence_id(service.imillion().NextSequenceId());

    // 发送到tlog服务
    service.imillion().Send(service.service_handle(), "tlog", "TLogWrite", event);

    return mjs::Value();
}

mjs::Value TLogModuleObject::GetStats(mjs::Context* context, uint32_t par_count, const mjs::StackFrame& stack) {
    auto& service = GetJSRuntineService(&context->runtime());

    // 发送获取统计信息请求
    auto req = std::make_shared<million::tlog::ss::TLogGetStats>();
    auto resp = service.imillion().Call<million::tlog::ss::TLogGetStatsResp>(
        service.service_handle(), "tlog", "TLogGetStats", req);

    // TODO: 将响应转换为JS对象返回
    return mjs::Value();
}

} // namespace jssvr
} // namespace million
