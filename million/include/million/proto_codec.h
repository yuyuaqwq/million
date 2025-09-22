#pragma once

#include <million/noncopyable.h>
#include <million/exception.h>
#include <million/message.h>
#include <million/net/packet.h>
#include <million/module_def.h>

namespace million {

using ProtoMessageKey = ModuleCode;
using ProtoMessageId = ModuleSubCode;
constexpr ProtoMessageId kProtoMessageIdInvalid = kModuleSubCodeInvalid;

class ProtoMgr;
class MILLION_API ProtoCodec : noncopyable {
public:
    ProtoCodec(const ProtoMgr& proto_mgr)
        : proto_mgr_(proto_mgr) {}

    // 注册协议
    template <typename ModuleExtIdT, typename MessageExtIdT>
    bool RegisterFile(const std::string& proto_file_name, ModuleExtIdT module_ext_id, MessageExtIdT msg_ext_id);

    std::optional<std::pair<ModuleId, ProtoMessageId>> FindMessageId(const protobuf::Message& message) const;

    std::optional<net::Packet> EncodeMessage(const protobuf::Message& message) const;
    struct DecodeRes {
        ModuleId module_id;
        ProtoMessageId msg_id;
        ProtoMessageUnique msg;
    };
    std::optional<DecodeRes> DecodeMessage(net::PacketSpan packet) const;

private:
    const protobuf::Descriptor* FindMessageDesc(ModuleId module_id, ProtoMessageId msg_id) const;
    std::optional<ProtoMessageKey> FindMessageKey(const protobuf::Descriptor* desc) const;

    std::optional<ProtoMessageUnique> NewMessage(ModuleId module_id, ProtoMessageId msg_id) const;

    uint32_t host_to_network_u32(uint32_t value) const;
    uint32_t network_to_host_u32(uint32_t value) const;

private:
    const ProtoMgr& proto_mgr_;

    std::unordered_map<ProtoMessageKey, const protobuf::Descriptor*> msg_desc_map_;
    std::unordered_map<const protobuf::Descriptor*, ProtoMessageKey> msg_id_map_;
};

inline net::Packet ProtoMsgToPacket(const google::protobuf::Message& msg) {
    auto packet = net::Packet(msg.ByteSize());
    if (!msg.SerializeToArray(packet.data(), packet.size())) {
        TaskAbort("Failed to serialize protobuf message to packet.");
    }
    return packet;
}

//#define MILLION_PROTO_PACKET_DISPATCH(PROTO_CODEC_, PROTO_PACKET_MSG_TYPE_) \
//    using _MILLION_PROTO_PACKET_MSG_TYPE_ = PROTO_PACKET_MSG_TYPE_; \
//    MILLION_MSG_HANDLE(PROTO_PACKET_MSG_TYPE_, msg) { \
//        auto res = (PROTO_CODEC_)->DecodeMessage(msg->packet); \
//        if (!res) { \
//            co_return; \
//        } \
//        co_await OnMsg(sender, session_id, std::move(res->msg)); \
//        co_return; \
//    } \

} // namespace million