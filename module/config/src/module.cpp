#include <iostream>
#include <fstream>
#include <filesystem>

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>
#include <million/config/config.h>
#include <million/config/ss_config.pb.h>
#include <million/config/config_options.pb.h>

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
        if (!imillion().SetServiceNameId(service_handle(), module::module_id, ss::ServiceNameId_descriptor(), ss::SERVICE_NAME_ID_CONFIG)) {
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

        const auto& tables_message_type_settings = config_settings["tables_message_type"];
        if (!tables_message_type_settings) {
            logger().LOG_ERROR("cannot find 'config.tables_message_type_settings'.");
            return false;
        }
        auto tables_message_type = namespace_ + "." + tables_message_type_settings.as<std::string>();
        auto table_desc = imillion().proto_mgr().FindMessageTypeByName(tables_message_type);
        if (!table_desc) {
            logger().LOG_ERROR("Unable to find message desc: tables_message_type -> {}.", tables_message_type);
            return false;
        }

        for (int i = 0; i < table_desc->field_count(); ++i) {
            auto field_desc = table_desc->field(i);
            if (!field_desc) {
                logger().LOG_ERROR("table_desc->field failed: {}.{}: Unable to retrieve field description.", tables_message_type, i);
                continue;
            }

            if (field_desc->type() != google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
                continue;
            }

            const auto* table_desc = field_desc->message_type();
            if (!table_desc) {
                logger().LOG_ERROR("field_desc->message_type() is nullptr: Field at index {} in message '{}'.", i, tables_message_type);
                continue;
            }

            std::string module_name;
            std::string pbb_file_name;
            const google::protobuf::FieldOptions& options = field_desc->options();

            auto* table_load_extension = &million::config::table_load; // 替换为你的实际扩展

            if (options.HasExtension(*table_load_extension)) {
                const auto& table_load_options = options.GetExtension(*table_load_extension);
                module_name = table_load_options.module_name();
                pbb_file_name = table_load_options.pbb_file_name();
            }
            else {
                logger().LOG_ERROR("Field '{}' in message '{}' does not have table_load option.",
                    field_desc->name(), tables_message_type);
                continue;
            }

            LoadConfig(module_name, pbb_file_name, *table_desc);
        }

        return true;
    }

    MILLION_MESSAGE_HANDLE(ConfigQueryReq, msg) {
        auto config_iter = config_map_.find(&msg->table_desc);
        if (config_iter == config_map_.end()) {
            co_return make_message<ConfigQueryResp>(msg->table_desc, std::nullopt);
        }

        co_return make_message<ConfigQueryResp>(msg->table_desc, ConfigTableWeakBase(config_iter->second));
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

    bool LoadConfig(std::string_view module_name, std::string_view pbb_file_name, const google::protobuf::Descriptor& table_desc) {
        // 加载对应文件
        std::filesystem::path pbb_path = pbb_dir_path_;
        pbb_path /= module_name;
        pbb_path /= std::string(pbb_file_name) + ".pbb";
        auto data = ReadPbb(pbb_path);
        if (!data) {
            logger().LOG_ERROR("ReadPbb failed: {}.", pbb_path.string());
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

        // logger().LOG_DEBUG("Table '{}' loaded:\n {}", table_desc.full_name(), config_msg->DebugString());

        config_map_[&table_desc] = std::make_shared<ConfigTableBase>(std::move(config_msg));

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