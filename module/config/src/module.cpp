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
private:
    struct Config {
        const google::protobuf::Descriptor* config_msg_desc;
        std::unordered_map<std::string, ProtoMsgShared> config_map;
    };

public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override {
        if (!imillion().SetServiceName(service_handle(), "ConfigService")) {
            return false;
        }

        const auto& settings = imillion().YamlSettings();
        const auto& config_settings = settings["config"];
        if (!config_settings) {
            logger().Err("cannot find 'config'.");
            return false;
        }

       

        const auto& pbb_dir_path_settings = config_settings["pbb_dir_path"];
        if (!pbb_dir_path_settings) {
            logger().Err("cannot find 'config.pbb_dir_path'.");
            return false;
        }
        pbb_dir_path_ = pbb_dir_path_settings.as<std::string>();

        const auto& namespace_settings = config_settings["namespace"];
        if (!namespace_settings) {
            logger().Err("cannot find 'config.namespace'.");
            return false;
        }
        auto namespace_ = namespace_settings.as<std::string>();


        const auto& modules_settings = config_settings["modules"];
        if (!modules_settings) {
            logger().Err("cannot find 'config.modules'.");
            return false;
        }
        for (auto module_settings : modules_settings) {
            Config config;
            auto module_ = module_settings.as<std::string>();
            auto config_msg_name = namespace_ + "." + module_ + ".Config";
            auto config_msg_desc = imillion().proto_mgr().FindMessageTypeByName(config_msg_name);
            if (!config_msg_desc) {
                logger().Err("Unable to find message: top_msg_name -> {}.", config_msg_name);
                return false;
            }
            config.config_msg_desc = config_msg_desc;
            for (int i = 0; i < config_msg_desc->field_count(); ++i) {
                auto field_desc = config_msg_desc->field(i);
                if (!field_desc) {
                    logger().Err("top_msg_desc->field failed: {}.{}: Unable to retrieve field description.", config_msg_name, i);
                    continue;
                }
                if (!field_desc->is_repeated()) {
                    logger().Err("top_msg_desc->field: Field at index {} in message '{}' is not repeated. Expected a repeated field.", i, config_msg_name);
                    continue;
                }

                const auto& name = field_desc->name();
                LoadConfig(module_, &config, name);
            }

            config_map_[module_] = std::move(config);
        }
        return true;
    }

    virtual void OnStop(ServiceHandle sender, SessionId session_id) override {
        
    }

    MILLION_MSG_DISPATCH(ConfigService);

    MILLION_MUT_MSG_HANDLE(ConfigQueryMsg, msg) {
        const auto& name = msg->config_desc.full_name();

        //auto iter = config_map_.find(name);
        //if (iter == config_map_.end()) {
        //    co_return std::move(msg_);
        //}

        //msg->config.emplace(iter->second);
        co_return std::move(msg_);
    }

    MILLION_MSG_HANDLE(ConfigUpdateMsg, msg) {
        //const auto& name = msg->config_desc.name();
        //if (!LoadConfig(name)) {
        //    logger().Err("LoadConfig failed: {}.", name);
        //}
        co_return std::move(msg_);
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

    bool LoadConfig(const std::string& module_, Config* config, const std::string& name) {
        // 加载对应文件
        std::filesystem::path pbb_path = pbb_dir_path_;
        pbb_path /= module_;
        pbb_path /= name + ".pbb";
        auto data = ReadPbb(pbb_path);
        if (!data) {
            logger().Err("ReadPbb failed: {}.{}.", module_, name);
            return false;
        }

        auto config_msg = imillion().proto_mgr().NewMessage(*config->config_msg_desc);
        if (!config_msg) {
            logger().Err("NewMessage failed: {}.{}.", module_, name);
            return false;
        }

        if (!config_msg->ParseFromArray(data->data(), data->size())) {
            logger().Err("ParseFromString failed: {}.{}.", module_, name);
            return false;
        }
        logger().Debug("Config '{}.{}' debug string:\n {}", module_, name, config_msg->DebugString());

        config->config_map[name] = ProtoMsgShared(config_msg.release());

        return true;
    }

private:
    std::string pbb_dir_path_;
    std::unordered_map<std::string, Config> config_map_;
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