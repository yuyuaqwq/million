#include <million/proto_mgr.h>

namespace million {

ProtoMgr::ProtoMgr()
    : codec_(*this) {}

void ProtoMgr::Init() {
    std::vector<std::string> file_names;
    desc_db().FindAllFileNames(&file_names);
    for (const std::string& file_name : file_names) {
        const protobuf::FileDescriptor* file_desc = desc_pool().FindFileByName(file_name);
    }

    std::vector<std::string> msg_names;
    desc_db().FindAllMessageNames(&msg_names);
    for (const std::string& msg_name : msg_names) {
        const protobuf::Descriptor* desc = desc_pool().FindMessageTypeByName(msg_name);
    }
}

const google::protobuf::DescriptorPool& ProtoMgr::desc_pool() {
    static auto desc_pool = google::protobuf::DescriptorPool::generated_pool();
    return *desc_pool;
}

google::protobuf::DescriptorDatabase& ProtoMgr::desc_db() {
    static auto desc_db = desc_pool().internal_generated_database();
    return *desc_db;
}

google::protobuf::MessageFactory& ProtoMgr::msg_factory() {
    static auto msg_factory = google::protobuf::MessageFactory::generated_factory();
    return *msg_factory;
}

const google::protobuf::FileDescriptor* ProtoMgr::FindFileByName(const std::string& name) const {
    return desc_pool().FindFileByName(name);
}

const google::protobuf::Descriptor* ProtoMgr::FindMessageTypeByName(const std::string& name) const {
    return desc_pool().FindMessageTypeByName(name);
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