#include <million/proto_mgr.h>

namespace million {

ProtoMgr::ProtoMgr()
    : codec_(*this) {}

void ProtoMgr::Init() {
    // 目前无意义，调试用，protogen dll可能延迟加载，所以这里没有定义属于正常现象
    // 保证所有使用protobuf的模块，都通过dll加载同一个protobuf
    // 即可确保google::protobuf::DescriptorPool::generated_pool()等接口获取的都是同一个对象
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

ProtoMessageUnique ProtoMgr::NewMessage(const google::protobuf::Descriptor& desc) const {
    auto prototype = GetPrototype(desc);
    if (!prototype) return nullptr;
    return ProtoMessageUnique(prototype->New());
}

} // namespace million