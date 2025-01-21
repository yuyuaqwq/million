#include <iostream>
#include <fstream>
#include <filesystem>

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <table/table.h>

MILLION_MODULE_INIT();

namespace million {
namespace table {

// 1.支持数据热更
// 2.访问配置即返回共享指针，热更只需要重新赋值共享指针即可



class TableService : public IService {
private:
    struct CfgTable {
        const google::protobuf::Descriptor* table_msg_desc;
        std::unordered_map<std::string, ProtoMsgShared> table_map;
    };

public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit(MsgPtr msg) override {
        if (!imillion().SetServiceName(service_handle(), "TableService")) {
            return false;
        }

        const auto& config = imillion().YamlConfig();
        const auto& table_config = config["table"];
        if (!table_config) {
            logger().Err("cannot find 'table'.");
            return false;
        }

       

        const auto& pbb_dir_path_config = table_config["pbb_dir_path"];
        if (!pbb_dir_path_config) {
            logger().Err("cannot find 'table.pbb_dir_path'.");
            return false;
        }
        pbb_dir_path_ = pbb_dir_path_config.as<std::string>();

        const auto& namespace_config = table_config["namespace"];
        if (!namespace_config) {
            logger().Err("cannot find 'table.namespace'.");
            return false;
        }
        auto namespace_ = namespace_config.as<std::string>();


        const auto& modules_config = table_config["modules"];
        if (!modules_config) {
            logger().Err("cannot find 'table.modules'.");
            return false;
        }
        for (auto module_config : modules_config) {
            CfgTable cfg_table;
            auto module_ = module_config.as<std::string>();
            auto table_msg_name = namespace_ + "." + module_ + ".Table";
            auto table_msg_desc = imillion().proto_mgr().FindMessageTypeByName(table_msg_name);
            if (!table_msg_desc) {
                logger().Err("Unable to find message: top_msg_name -> {}.", table_msg_name);
                return false;
            }
            cfg_table.table_msg_desc = table_msg_desc;
            for (int i = 0; i < table_msg_desc->field_count(); ++i) {
                auto field_desc = table_msg_desc->field(i);
                if (!field_desc) {
                    logger().Err("top_msg_desc->field failed: {}.{}: Unable to retrieve field description.", table_msg_name, i);
                    continue;
                }
                if (!field_desc->is_repeated()) {
                    logger().Err("top_msg_desc->field: Field at index {} in message '{}' is not repeated. Expected a repeated field.", i, table_msg_name);
                    continue;
                }

                const auto& name = field_desc->name();
                LoadTable(module_, &cfg_table, name);
            }

            cfg_table_map_[module_] = std::move(cfg_table);
        }
        return true;
    }

    virtual void OnStop(ServiceHandle sender, SessionId session_id) override {
        
    }

    MILLION_MSG_DISPATCH(TableService);

    MILLION_MUT_MSG_HANDLE(TableQueryMsg, msg) {
        const auto& name = msg->table_desc.full_name();

        //auto iter = table_map_.find(name);
        //if (iter == table_map_.end()) {
        //    co_return std::move(msg_);
        //}

        //msg->table.emplace(iter->second);
        co_return std::move(msg_);
    }

    MILLION_MSG_HANDLE(TableUpdateMsg, msg) {
        //const auto& name = msg->table_desc.name();
        //if (!LoadTable(name)) {
        //    logger().Err("LoadTable failed: {}.", name);
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

    bool LoadTable(const std::string& module_, CfgTable* cfg_table, const std::string& name) {
        // 加载对应文件
        std::filesystem::path pbb_path = pbb_dir_path_;
        pbb_path /= module_;
        pbb_path /= name + ".pbb";
        auto data = ReadPbb(pbb_path);
        if (!data) {
            logger().Err("ReadPbb failed: {}.{}.", module_, name);
            return false;
        }

        auto table_msg = imillion().proto_mgr().NewMessage(*cfg_table->table_msg_desc);
        if (!table_msg) {
            logger().Err("NewMessage failed: {}.{}.", module_, name);
            return false;
        }

        if (!table_msg->ParseFromArray(data->data(), data->size())) {
            logger().Err("ParseFromString failed: {}.{}.", module_, name);
            return false;
        }
        logger().Debug("Table '{}.{}' debug string:\n {}", module_, name, table_msg->DebugString());

        cfg_table->table_map[name] = ProtoMsgShared(table_msg.release());

        return true;
    }

private:
    std::string pbb_dir_path_;
    std::unordered_map<std::string, CfgTable> cfg_table_map_;
};

extern "C" MILLION_TABLE_API bool MillionModuleInit(IMillion* imillion) {
    auto& config = imillion->YamlConfig();
    
    auto table_service_opt = imillion->NewService<TableService>();
    if (!table_service_opt) {
        return false;
    }

    return true;
}

} // namespace db
} // namespace million