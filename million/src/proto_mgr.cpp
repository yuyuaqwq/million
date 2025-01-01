#include <million/proto_mgr.h>

#include <protogen/protogen.h>

namespace million {

ProtoMgr::ProtoMgr()
    : codec_(*this) {}

void ProtoMgr::Init() {
    ProtogenInit();
}


const google::protobuf::DescriptorPool& ProtoMgr::desc_pool() {
    static const auto* pool = google::protobuf::DescriptorPool::generated_pool();
    return *pool;
}

google::protobuf::DescriptorDatabase& ProtoMgr::desc_db() {
    static auto* db = desc_pool().internal_generated_database();
    return *db;
}

google::protobuf::MessageFactory& ProtoMgr::msg_factory() {
    static auto* factory = google::protobuf::MessageFactory::generated_factory();
    return *factory;
}

const google::protobuf::FileDescriptor* ProtoMgr::FindFileByName(const std::string& proto_file_name) const {
    std::vector<std::string> file_names;
    desc_db().FindAllFileNames(&file_names);   // 遍历得到所有proto文件名
    for (const std::string& filename : file_names) {
        const protobuf::FileDescriptor* file_desc = desc_pool().FindFileByName(filename);
    }

    auto file_desc = desc_pool().FindFileByName(proto_file_name);
    if (file_desc) {
        return file_desc;
    }
    
    return nullptr;
}

const google::protobuf::Message* ProtoMgr::GetPrototype(const google::protobuf::Descriptor& desc) const {
    return msg_factory().GetPrototype(&desc);
}

ProtoMsgUnique ProtoMgr::NewMessage(const google::protobuf::Descriptor& desc) const {
    auto prototype = GetPrototype(desc);
    if (!prototype) return nullptr;
    return ProtoMsgUnique(prototype->New());
}

} // namespace million