#include "iostream"

#include <atomic>
#include <chrono>
#include <optional>
#include <mutex>
#include <queue>
#include <random>
#include <sstream>
#include <iomanip>

#include <million/imillion.h>
#include <million/imsg.h>

#include <protogen/cs/cs_msgid.pb.h>
#include <protogen/cs/cs_user.pb.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>

#include <yaml-cpp/yaml.h>

#include "net/server.h"

class TokenGenerator {
public:
    TokenGenerator()
        : gen_(rd_())
        , dis_(0, std::numeric_limits<uint32_t>::max()) {}

    uint64_t generate_token() {
        auto now = std::chrono::system_clock::now();
        uint32_t timestamp = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());

        uint32_t random_value = dis_(gen_);

        // 64位token
        uint64_t token = (static_cast<uint64_t>(timestamp) << 32) | random_value;
        return token;
    }

private:
    std::random_device rd_;
    std::mt19937_64 gen_;
    std::uniform_int_distribution<uint32_t> dis_;
};

class ProtoManager {
public:
    // 初始化消息ID
    void Init() {
        const google::protobuf::DescriptorPool* pool = google::protobuf::DescriptorPool::generated_pool();
        google::protobuf::DescriptorDatabase* db = pool->internal_generated_database();
        if (db == nullptr) {
            return;
        }

        std::vector<std::string> file_names;
        db->FindAllFileNames(&file_names);   // 遍历得到所有proto文件名
        for (const std::string& filename : file_names) {
            const google::protobuf::FileDescriptor* file_descriptor = pool->FindFileByName(filename);
            if (file_descriptor == nullptr) continue;

            // 获取该文件中定义的 SubMsgId Enum
            int enum_count = file_descriptor->enum_type_count();
            const google::protobuf::EnumDescriptor* enum_descriptor = nullptr;
            for (int i = 0; i < enum_count; i++) {
                const google::protobuf::EnumDescriptor* descriptor = file_descriptor->enum_type(i);
                if (!descriptor) continue;
                const std::string& name = descriptor->full_name();
                auto& opts = descriptor->options();
                if (opts.HasExtension(Cs::cs_msg_id)) {
                    Cs::MsgId msg_id = opts.GetExtension(Cs::cs_msg_id);

                    // 建立这个文件与对应的msg_id的映射
                    file_desc_map_.insert(std::make_pair(msg_id, file_descriptor));
                    break;
                }
            }
        }
    }

    // 注册子消息
    template <typename ExtensionIdT>
    bool RegistrySubMsgId(Cs::MsgId msg_id, const ExtensionIdT& id) {
        auto iter = file_desc_map_.find(msg_id);
        if (iter == file_desc_map_.end()) { return false; }
        const google::protobuf::FileDescriptor* file_descriptor = iter->second;

        int message_count = file_descriptor->message_type_count();
        for (int i = 0; i < message_count; i++) {
            const google::protobuf::Descriptor* descriptor = file_descriptor->message_type(i);
            if (!descriptor) continue;
            const std::string& name = descriptor->full_name();
            auto& options = descriptor->options();
            if (options.HasExtension(id)) {
                auto sub_msg_id = options.GetExtension(id);
                static_assert(sizeof(Cs::MsgId) == sizeof(sub_msg_id), "");
                uint64_t key = (static_cast<uint64_t>(msg_id) << 32) | static_cast<uint32_t>(sub_msg_id);
                msg_desc_map_.insert(std::make_pair(key, descriptor));
            }
        }
    }

    template <typename SubMsgId>
    const google::protobuf::Descriptor* GetMsgDesc(Cs::MsgId msg_id, SubMsgId sub_msg_id) {
        static_assert(sizeof(Cs::MsgId) == sizeof(sub_msg_id), "");
        uint64_t key = (static_cast<uint64_t>(msg_id) << 32) | static_cast<uint32_t>(sub_msg_id);
        auto iter = msg_desc_map_.find(key);
        if (iter == msg_desc_map_.end()) return nullptr;
        return iter->second;
    }

private:
    static_assert(sizeof(Cs::MsgId) == sizeof(uint32_t), "");
    std::unordered_map<Cs::MsgId, const google::protobuf::FileDescriptor*> file_desc_map_;
    std::unordered_map<uint64_t, const google::protobuf::Descriptor*> msg_desc_map_;
};


class GateWayService : public million::IService {
public:
    using Base = million::IService;
    GateWayService(million::IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual void OnInit() override {
        server_.set_on_connection([](auto connection_handle) {
            std::cout << "on_connection" << std::endl;
        });
        server_.set_on_msg([](auto& connection, auto&& packet) {
            std::cout << "on_msg" << std::endl;
        });

        server_.Start(8001);
    }

    virtual million::Task OnMsg(million::MsgUnique msg) override {
        
        co_return;
    }

private:
    million::net::Server server_;
};



MILLION_FUNC_EXPORT bool MillionModuleInit(million::IMillion* imillion) {
    // 初始化 Protobuf 库
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    ProtoManager mgr;
    mgr.Init();
    mgr.RegistrySubMsgId(Cs::MSG_ID_USER, Cs::cs_sub_msg_id_user);

    auto desc = mgr.GetMsgDesc(Cs::MSG_ID_USER, Cs::SUB_MSG_ID_USER_LOGIN);

    // 清理 Protobuf 资源
    // google::protobuf::ShutdownProtobufLibrary();


    auto& config = imillion->config();
    auto handle = imillion->NewService<GateWayService>();



    return true;
}
