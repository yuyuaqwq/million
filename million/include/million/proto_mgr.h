#pragma once

#include <million/api.h>
#include <million/proto_codec.h>

namespace million {

class MILLION_API ProtoMgr {
public:
    ProtoMgr();

    void Init();

    const google::protobuf::FileDescriptor* FindFileByName(const std::string& name) const;
    const google::protobuf::Descriptor* FindMessageTypeByName(const std::string& name) const;
    const google::protobuf::EnumDescriptor* FindEnumTypeByName(const std::string& name) const;
    int32_t FindEnumValueByFullName(const std::string& service_id_full_name);

    const google::protobuf::Message* GetPrototype(const google::protobuf::Descriptor& desc) const;

    ProtoMessageUnique NewMessage(const google::protobuf::Descriptor& desc) const;

    const ProtoCodec& codec() const { return codec_; }
    ProtoCodec& codec() { return codec_; }

private:
    static const google::protobuf::DescriptorPool& desc_pool();
    static google::protobuf::DescriptorDatabase& desc_db();
    static google::protobuf::MessageFactory& msg_factory();

private:
    ProtoCodec codec_;
};


template <typename ModuleExtIdT, typename MessageExtIdT>
bool ProtoCodec::RegisterFile(const std::string& proto_file_name, ModuleExtIdT module_ext_id, MessageExtIdT msg_ext_id) {
    auto file_desc = proto_mgr_.FindFileByName(proto_file_name);
    if (!file_desc) {
        return false;
    }

    int enum_count = file_desc->enum_type_count();
    int i = 0;

    ModuleId module_id_ui = 0;
    for (; i < enum_count; i++) {
        const auto* enum_desc = file_desc->enum_type(i);
        if (!enum_desc) continue;
        auto& enum_opts = enum_desc->options();
        if (!enum_opts.HasExtension(module_ext_id)) {
            continue;
        }

        auto module_id = enum_opts.GetExtension(module_ext_id);
        module_id_ui = static_cast<ModuleId>(module_id);
        if (module_id_ui > std::numeric_limits<ModuleId>::max()) {
            return false;
        }
        break;
    }
    if (i >= enum_count) {
        return false;
    }

    int message_count = file_desc->message_type_count();
    for (int j = 0; j < message_count; j++) {
        const auto* desc = file_desc->message_type(j);

        auto& msg_opts = desc->options();
        if (!msg_opts.HasExtension(msg_ext_id)) {
            continue;
        }

        auto msg_id = msg_opts.GetExtension(msg_ext_id);
        auto msg_id_ui = static_cast<ProtoMessageId>(msg_id);
        if (msg_id_ui > std::numeric_limits<ProtoMessageId>::max()) {
            return false;
        }

        auto key = EncodeModuleCode(module_id_ui, msg_id_ui);
        msg_desc_map_.emplace(key, desc);
        msg_id_map_.emplace(desc, key);
    }

    return true;
}

} // namespace million