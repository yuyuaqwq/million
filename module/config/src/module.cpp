#include <iostream>
#include <fstream>
#include <filesystem>

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <config/config.h>

MILLION_MODULE_INIT();

namespace million {
namespace config {

// 1.通过共享指针支持数据热更
// 2.访问配置即返回共享指针，热更只需要重新赋值共享指针即可

class ConfigService : public IService {
    MILLION_SERVICE_DEFINE(ConfigService);

public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override {
        if (!imillion().SetServiceName(service_handle(), kConfigServiceName)) {
            return false;
        }

        const auto& settings = imillion().YamlSettings();
        const auto& config_settings = settings["config"];
        if (!config_settings) {
            logger().LOG_ERROR("cannot find 'config'.");
            return false;
        }

        const auto& pbb_dir_path_settings = config_settings["pbb_dir_path"];
        if (!pbb_dir_path_settings) {
            logger().LOG_ERROR("cannot find 'config.pbb_dir_path'.");
            return false;
        }
        pbb_dir_path_ = pbb_dir_path_settings.as<std::string>();

        const auto& namespace_settings = config_settings["namespace"];
        if (!namespace_settings) {
            logger().LOG_ERROR("cannot find 'config.namespace'.");
            return false;
        }
        auto namespace_ = namespace_settings.as<std::string>();

        const auto& modules_settings = config_settings["modules"];
        if (!modules_settings) {
            logger().LOG_ERROR("cannot find 'config.modules'.");
            return false;
        }
        for (auto module_settings : modules_settings) {
            auto module_name = module_settings.as<std::string>();
            auto table_msg_name = namespace_ + ".config." + module_name + ".Table";
            auto table_desc = imillion().proto_mgr().FindMessageTypeByName(table_msg_name);
            if (!table_desc) {
                logger().LOG_ERROR("Unable to find message desc: top_msg_name -> {}.", table_msg_name);
                return false;
            }

            for (int i = 0; i < table_desc->field_count(); ++i) {
                auto field_desc = table_desc->field(i);
                if (!field_desc) {
                    logger().LOG_ERROR("table_desc->field failed: {}.{}: Unable to retrieve field description.", table_msg_name, i);
                    continue;
                }

                if (!field_desc->is_repeated()) {
                    logger().LOG_ERROR("table_desc->field: Field at index {} in message '{}' is not repeated. Expected a repeated field.", i, table_msg_name);
                    continue;
                }

                if (field_desc->type() != google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
                    continue;
                }

                const auto* config_desc = field_desc->message_type();
                if (!config_desc) {
                    logger().LOG_ERROR("field_desc->message_type() is nullptr: Field at index {} in message '{}'.", i, table_msg_name);
                    continue;
                }
                LoadConfig(module_name, *table_desc, *config_desc);
            }

        }
        return true;
    }

    MILLION_MESSAGE_HANDLE(ConfigQueryReq, msg) {
        auto config_iter = config_map_.find(&msg->config_desc);
        if (config_iter == config_map_.end()) {
            co_return make_message<ConfigQueryResp>(msg->config_desc, std::nullopt);
        }

        

        co_return make_message<ConfigQueryResp>(msg->config_desc, ConfigTableWeakBase(config_iter->second));
    }

    MILLION_MESSAGE_HANDLE(const ConfigUpdateReq, msg) {
        //const auto& name = msg->config_desc.name();
        //if (!LoadConfig(name, )) {
        //    logger().LOG_ERROR("LoadConfig failed: {}.", name);
        //}
        co_return make_message<ConfigUpdateResp>();
    }

private:
    std::optional<std::vector<char>> ReadPbb(const std::filesystem::path& pbb_path) {
        auto file = std::ifstream(pbb_path, std::ios::binary);
        if (!file) {
            return std::nullopt;
        }

        // 获取文件大小
        file.seekg(0, std::ios::end);
        auto size = file.tellg();
        file.seekg(0, std::ios::beg);

        // 读取文件内容到vector中
        std::vector<char> content(size);
        file.read(content.data(), size);
        file.close();

        return content;
    }

    bool LoadConfig(const std::string module_name, const google::protobuf::Descriptor& table_desc, const google::protobuf::Descriptor& config_desc) {
        // 加载对应文件
        std::filesystem::path pbb_path = pbb_dir_path_;
        pbb_path /= module_name;
        pbb_path /= config_desc.name() + ".pbb";
        auto data = ReadPbb(pbb_path);
        if (!data) {
            logger().LOG_ERROR("ReadPbb failed: {}.", config_desc.full_name());
            return false;
        }

        auto config_msg = imillion().proto_mgr().NewMessage(table_desc);
        if (!config_msg) {
            logger().LOG_ERROR("NewMessage failed: {}.", table_desc.full_name());
            return false;
        }

        if (!config_msg->ParseFromArray(data->data(), data->size())) {
            logger().LOG_ERROR("ParseFromString failed: {}.", table_desc.full_name());
            return false;
        }
        // logger().LOG_DEBUG("Config '{}.{}' debug string:\n {}", config_desc.full_name(), config_desc.full_name(), config_msg->DebugString());

        config_map_[&config_desc] = std::make_shared<ConfigTableBase>(std::move(config_msg), &config_desc);

        return true;
    }

private:
    std::string pbb_dir_path_;
    std::unordered_map<const google::protobuf::Descriptor*, ConfigTableShared> config_map_;
};

extern "C" MILLION_CONFIG_API bool MillionModuleInit(IMillion* imillion) {
    auto& settings = imillion->YamlSettings();
    
    auto config_service_opt = imillion->NewService<ConfigService>();
    if (!config_service_opt) {
        return false;
    }

    return true;
}

} // namespace db
} // namespace million