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

    ProtoMsgUnique NewMessage(const google::protobuf::Descriptor& desc) const;

    const ProtoCodec& codec() const { return codec_; }
    ProtoCodec& codec() { return codec_; }

private:
    const google::protobuf::Message* GetPrototype(const google::protobuf::Descriptor& desc) const;

    static const google::protobuf::DescriptorPool& desc_pool();
    static google::protobuf::DescriptorDatabase& desc_db();
    static google::protobuf::MessageFactory& msg_factory();

private:
    ProtoCodec codec_;
};


template <typename MsgExtIdT, typename SubMsgExtIdT>
bool ProtoCodec::RegisterFile(const std::string& proto_file_name, MsgExtIdT msg_ext_id, SubMsgExtIdT sub_msg_ext_id) {
    auto file_desc = proto_mgr_.FindFileByName(proto_file_name);
    if (!file_desc) {
        return false;
    }

    int enum_count = file_desc->enum_type_count();
    int i = 0;

    MsgId msg_id_ui = 0;
    for (; i < enum_count; i++) {
        const auto* enum_desc = file_desc->enum_type(i);
        if (!enum_desc) continue;
        auto& enum_opts = enum_desc->options();
        if (!enum_opts.HasExtension(msg_ext_id)) {
            continue;
        }

        auto msg_id = enum_opts.GetExtension(msg_ext_id);
        msg_id_ui = static_cast<MsgId>(msg_id);
        if (msg_id_ui > kMsgIdMax) {
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
        if (!msg_opts.HasExtension(sub_msg_ext_id)) {
            continue;
        }

        auto sub_msg_id = msg_opts.GetExtension(sub_msg_ext_id);
        auto sub_msg_id_ui = static_cast<SubMsgId>(sub_msg_id);
        if (sub_msg_id_ui > kSubMsgIdMax) {
            return false;
        }

        auto key = CalcKey(msg_id_ui, sub_msg_id_ui);
        msg_desc_map_.emplace(key, desc);
        msg_id_map_.emplace(desc, key);
    }

    return true;
}

} // namespace million