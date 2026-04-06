#pragma once

#include <mjs/runtime.h>
#include <million/imillion.h>
#include <million/config/config.h>

namespace million {
namespace jssvr {

class JSRuntimeService;

// JS配置查询消息
MILLION_MESSAGE_DEFINE(, JSConfigQueryReq, (const google::protobuf::Descriptor&) config_desc)
MILLION_MESSAGE_DEFINE(, JSConfigQueryResp, (mjs::Value) cached_table)

// JSConfigService - 预转换缓存配置服务
class JSConfigService : public IService {
    MILLION_SERVICE_DEFINE(JSConfigService);

public:
    using Base = IService;
    JSConfigService(IMillion* imillion, JSRuntimeService* js_runtime_service);
    ~JSConfigService();

    MILLION_MESSAGE_HANDLE(config::ConfigUpdateReq, msg) {
        co_await UpdateConfigCache(&msg->table_desc);
        co_return make_message<config::ConfigUpdateResp>();
    }

    MILLION_MESSAGE_HANDLE(JSConfigQueryReq, msg) {
        // 创建临时上下文用于获取配置
        auto cached_table = co_await GetCachedConfigTable(&msg->config_desc);
        co_return make_message<JSConfigQueryResp>(std::move(cached_table));
    }

private:
    virtual bool OnInit() override;
    virtual Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override;


    // 预加载所有配置
    Task<bool> PreloadAllConfigs();

    // 获取缓存的配置表JS对象
    Task<mjs::Value> GetCachedConfigTable(const google::protobuf::Descriptor* descriptor);

    // 更新特定配置的缓存
    Task<void> UpdateConfigCache(const google::protobuf::Descriptor* descriptor);

    // 后续通过注册配置更新回调

private:
    JSRuntimeService* js_runtime_service_;
    ServiceHandle config_service_handle_;

    // 缓存的JS配置表对象 - 使用descriptor作为key
    std::unordered_map<const google::protobuf::Descriptor*, mjs::Value> cached_config_tables_;

    // 配置名称到descriptor的映射
    std::unordered_map<std::string, const google::protobuf::Descriptor*> config_name_to_descriptor_;
};

} // namespace jssvr
} // namespace million
