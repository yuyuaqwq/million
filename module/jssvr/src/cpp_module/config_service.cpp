#include "config_service.h"

#include <million/jssvr/jssvr.h>
#include <million/config/ss_config.pb.h>

#include "../js_service.h"
#include "../js_util.h"
#include "config_module.h"

namespace million {
namespace jssvr {

JSConfigService::JSConfigService(IMillion* imillion, JSRuntimeService* js_runtime_service)
    : Base(imillion)
    , js_runtime_service_(js_runtime_service) {
}

JSConfigService::~JSConfigService() = default;

bool JSConfigService::OnInit() {
    // 获取配置服务句柄
    auto config_service_opt = imillion().FindServiceByNameId(module::module_id, config::ss::ServiceNameId_descriptor(), config::ss::SERVICE_NAME_ID_CONFIG);
    if (!config_service_opt) {
        logger().LOG_ERROR("Config service not found.");
        return false;
    }
    config_service_handle_ = *config_service_opt;

    return true;
}

Task<MessagePointer> JSConfigService::OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) {
    // 预加载所有配置
    co_await PreloadAllConfigs();

    logger().LOG_INFO("JSConfigService started and configs preloaded.");
    co_return nullptr;
}

Task<bool> JSConfigService::PreloadAllConfigs() {
    // 获取所有已注册的配置类型
    const auto& settings = imillion().YamlSettings();
    const auto& config_settings = settings["config"];
    if (!config_settings) {
        logger().LOG_ERROR("cannot find 'config'.");
        co_return false;
    }

    const auto& namespace_settings = config_settings["namespace"];
    if (!namespace_settings) {
        logger().LOG_ERROR("cannot find 'config.namespace'.");
        co_return false;
    }
    auto namespace_ = namespace_settings.as<std::string>();

    const auto& tables_message_type_settings = config_settings["tables_message_type"];
    if (!tables_message_type_settings) {
        logger().LOG_ERROR("cannot find 'config.tables_message_type'.");
        co_return false;
    }
    auto tables_message_type = namespace_ + "." + tables_message_type_settings.as<std::string>();
    auto tables_desc = imillion().proto_mgr().FindMessageTypeByName(tables_message_type);
    if (!tables_desc) {
        logger().LOG_ERROR("Unable to find message desc: tables_message_type -> {}.", tables_message_type);
        co_return false;
    }

    for (int i = 0; i < tables_desc->field_count(); ++i) {
        auto field_desc = tables_desc->field(i);
        if (!field_desc || field_desc->type() != google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
            continue;
        }

        const auto* table_desc = field_desc->message_type();
        if (!table_desc) {
            continue;
        }

        // 预加载这个配置类型
        try {
            auto config_resp = co_await Call<config::ConfigQueryReq, config::ConfigQueryResp>(config_service_handle_, *table_desc);
            if (!config_resp->config) {
                logger().LOG_ERROR("Failed to query config: {}", table_desc->full_name());
                continue;
            }

            auto config_table = co_await config_resp->config->Lock(this, config_service_handle_, table_desc);

            if (config_table) {
                // 预转换所有行为JS对象
                std::vector<mjs::Value> cached_rows;
                for (size_t row_idx = 0; row_idx < config_table->GetRowCount(); ++row_idx) {
                    const auto* row = config_table->GetRowByIndex(row_idx);
                    if (row) {
                        auto js_row = JSUtil::ProtoMessageToJSObject(&js_runtime_service_->js_runtime(), *row);
                        cached_rows.push_back(std::move(js_row));
                    }
                }

                logger().LOG_INFO("Preloaded config: {} with {} rows", table_desc->full_name(), cached_rows.size());

                auto cached_table = ConfigTableObject::New(&js_runtime_service_->js_runtime(), table_desc, std::move(cached_rows));
                cached_config_tables_[table_desc] = mjs::Value(cached_table);
                config_name_to_descriptor_[table_desc->full_name()] = table_desc;
            }
        }
        catch (const std::exception& e) {
            logger().LOG_ERROR("Failed to preload config {}: {}", table_desc->full_name(), e.what());
        }
    }


    co_return true;
}

Task<mjs::Value> JSConfigService::GetCachedConfigTable(const google::protobuf::Descriptor* descriptor) {
    // 尝试从缓存读取
    auto it = cached_config_tables_.find(descriptor);
    if (it != cached_config_tables_.end()) {
        co_return it->second;
    }
    co_return mjs::Value(); // 不存在返回undefined
}

Task<void> JSConfigService::UpdateConfigCache(const google::protobuf::Descriptor* descriptor) {
    // 配置更新时重新缓存
    logger().LOG_INFO("Config cache updated for: {}", descriptor->full_name());

    co_return;
}

} // namespace jssvr
} // namespace million
