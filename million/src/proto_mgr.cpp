#include <million/proto_mgr.h>

namespace million {

ProtoMgr::ProtoMgr()
    : codec_(*this) {}

void ProtoMgr::Init() {
    RegisterMeta(GetProtoMeta());
}

void ProtoMgr::RegisterMeta(const ProtoMeta& meta) {
    meta_list_.emplace_back(meta);
}

const google::protobuf::FileDescriptor* ProtoMgr::FindFileByName(const std::string& proto_file_name) const {
    for (const auto& meta : meta_list_) {
        std::vector<std::string> file_names;
        meta.desc_db().FindAllFileNames(&file_names);   // 遍历得到所有proto文件名
        for (const std::string& filename : file_names) {
            const protobuf::FileDescriptor* file_desc = meta.desc_pool().FindFileByName(filename);
        }

        auto file_desc = meta.desc_pool().FindFileByName(proto_file_name);
        if (file_desc) {
            return file_desc;
        }
    }
    return nullptr;
}

const google::protobuf::Message* ProtoMgr::GetPrototype(const google::protobuf::Descriptor& desc) const {
    for (const auto& meta : meta_list_) {
        auto prototype = meta.msg_factory().GetPrototype(&desc);
        if (prototype) {
            return prototype;
        }
    }
    return nullptr;
}

ProtoMsgUnique ProtoMgr::NewMessage(const google::protobuf::Descriptor& desc) const {
    auto prototype = GetPrototype(desc);
    if (!prototype) return nullptr;
    return ProtoMsgUnique(prototype->New());
}

} // namespace million