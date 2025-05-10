#pragma once

#include <million/noncopyable.h>
#include <million/exception.h>
#include <million/message.h>
#include <million/net/packet.h>

namespace million {

using MsgKey = uint32_t;
using MsgId = uint16_t;
using SubMsgId = uint16_t;

class ProtoMgr;
class MILLION_API ProtoCodec : noncopyable {
public:
    ProtoCodec(const ProtoMgr& proto_mgr)
        : proto_mgr_(proto_mgr) {}

    // ×¢²áÐ­Òé
    template <typename MsgExtIdT, typename SubMsgExtIdT>
    bool RegisterFile(const std::string& proto_file_name, MsgExtIdT msg_ext_id, SubMsgExtIdT sub_msg_ext_id);

    std::optional<std::pair<MsgId, SubMsgId>> GetMsgId(const protobuf::Message& message) const;

    std::optional<net::Packet> EncodeMessage(const protobuf::Message& message) const;
    struct DecodeRes {
        MsgId msg_id;
        SubMsgId sub_msg_id;
        ProtoMessageUnique msg;
    };
    std::optional<DecodeRes> DecodeMessage(net::PacketSpan packet) const;

    static MsgKey CalcKey(MsgId msg_id, SubMsgId sub_msg_id);
    static std::pair<MsgId, SubMsgId> CalcMsgId(MsgKey key);

private:
    const protobuf::Descriptor* GetMsgDesc(MsgId msg_id, SubMsgId sub_msg_id) const;
    std::optional<MsgKey> GetMsgKey(const protobuf::Descriptor* desc) const;

    std::optional<ProtoMessageUnique> NewMessage(MsgId msg_id, SubMsgId sub_msg_id) const;

    uint16_t host_to_network_short(uint16_t value) const;
    uint16_t network_to_host_short(uint16_t value) const;

private:
    constexpr static inline MsgId kMsgIdMax = std::numeric_limits<MsgId>::max();
    constexpr static inline SubMsgId kSubMsgIdMax = std::numeric_limits<SubMsgId>::max();

    const ProtoMgr& proto_mgr_;

    std::unordered_map<MsgKey, const protobuf::Descriptor*> msg_desc_map_;
    std::unordered_map<const protobuf::Descriptor*, MsgKey> msg_id_map_;
};

inline net::Packet ProtoMsgToPacket(const google::protobuf::Message& msg) {
    auto packet = net::Packet(msg.ByteSize());
    if (!msg.SerializeToArray(packet.data(), packet.size())) {
        throw TaskAbortException("Failed to serialize protobuf message to packet.");
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