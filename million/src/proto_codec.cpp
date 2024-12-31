#include <million/proto_codec.h>

#include <million/proto_mgr.h>

namespace million {

std::optional<std::pair<MsgId, SubMsgId>> ProtoCodec::GetMsgId(const protobuf::Message& message) const {
    auto key = GetMsgKey(message.GetDescriptor());
    if (!key) return std::nullopt;
    return CalcMsgId(*key);
}

std::optional<net::Packet> ProtoCodec::EncodeMessage(const protobuf::Message& message) const {
    auto desc = message.GetDescriptor();
    auto msg_key = GetMsgKey(desc);
    if (!msg_key) {
        return std::nullopt;
    }
    auto [msg_id, sub_msg_id] = CalcMsgId(*msg_key);

    MsgId msg_id_net = host_to_network_short(static_cast<uint16_t>(msg_id));
    SubMsgId sub_msg_id_net = host_to_network_short(static_cast<uint16_t>(sub_msg_id));

    auto packet = net::Packet(sizeof(msg_id_net) + sizeof(sub_msg_id_net) + message.ByteSize());
    size_t i = 0;
    std::memcpy(packet.data() + i, &msg_id_net, sizeof(msg_id_net));
    i += sizeof(msg_id_net);
    std::memcpy(packet.data() + i, &sub_msg_id_net, sizeof(sub_msg_id_net));
    i += sizeof(sub_msg_id_net);

    message.SerializeToArray(packet.data() + i, message.ByteSize());
    return packet;
}


std::optional<ProtoCodec::DecodeRes> ProtoCodec::DecodeMessage(net::PacketSpan packet) const {
    DecodeRes res = { 0 };

    MsgId msg_id_net;
    SubMsgId sub_msg_id_net;
    if (packet.size() < sizeof(msg_id_net) + sizeof(sub_msg_id_net)) return std::nullopt;

    size_t i = 0;
    std::memcpy(&msg_id_net, packet.data() + i, sizeof(msg_id_net));
    i += sizeof(msg_id_net);
    std::memcpy(&sub_msg_id_net, packet.data() + i, sizeof(sub_msg_id_net));
    i += sizeof(sub_msg_id_net);

    res.msg_id = static_cast<MsgId>(network_to_host_short(msg_id_net));
    res.sub_msg_id = static_cast<SubMsgId>(network_to_host_short(sub_msg_id_net));
    if (res.msg_id > kMsgIdMax) {
        return std::nullopt;
    }
    if (res.sub_msg_id > kMsgIdMax) {
        return std::nullopt;
    }

    auto msg_opt = NewMessage(res.msg_id, res.sub_msg_id);
    if (!msg_opt) return {};
    res.msg = std::move(*msg_opt);

    auto success = res.msg.GetProtoMessage()->ParseFromArray(packet.data() + i, packet.size() - i);
    if (!success) {
        return std::nullopt;
    }
    return res;
}

MsgKey ProtoCodec::CalcKey(MsgId msg_id, SubMsgId sub_msg_id) {
    assert(static_cast<MsgKey>(msg_id) <= kMsgIdMax);
    assert(sub_msg_id <= kSubMsgIdMax);
    return MsgKey(static_cast<MsgKey>(msg_id) << sizeof(sub_msg_id) * 8) | static_cast<MsgKey>(sub_msg_id);
}

std::pair<MsgId, SubMsgId> ProtoCodec::CalcMsgId(MsgKey key) {
    auto msg_id = key >> 16;
    auto sub_msg_id = key & 0xffff;
    return std::make_pair(msg_id, sub_msg_id);
}

const protobuf::Descriptor* ProtoCodec::GetMsgDesc(MsgId msg_id, SubMsgId sub_msg_id) const {
    auto key = CalcKey(msg_id, sub_msg_id);
    auto iter = msg_desc_map_.find(key);
    if (iter == msg_desc_map_.end()) return nullptr;
    return iter->second;
}

std::optional<MsgKey> ProtoCodec::GetMsgKey(const protobuf::Descriptor* desc) const {
    auto iter = msg_id_map_.find(desc);
    if (iter == msg_id_map_.end()) return std::nullopt;
    return iter->second;
}

std::optional<MsgUnique> ProtoCodec::NewMessage(MsgId msg_id, SubMsgId sub_msg_id) const {
    auto desc = GetMsgDesc(msg_id, sub_msg_id);
    if (!desc) return std::nullopt;
    const ProtoMessage* msg = proto_mgr_.GetPrototype(*desc);
    if (msg != nullptr) {
        return MsgUnique(msg->New());
    }
    return std::nullopt;
}

uint16_t ProtoCodec::host_to_network_short(uint16_t value) const {
    uint16_t result;
    unsigned char* result_p = reinterpret_cast<unsigned char*>(&result);
    result_p[0] = static_cast<unsigned char>((value >> 8) & 0xFF);
    result_p[1] = static_cast<unsigned char>(value & 0xFF);
    return result;
}

uint16_t ProtoCodec::network_to_host_short(uint16_t value) const {
    uint16_t result;
    unsigned char* result_p = reinterpret_cast<unsigned char*>(&result);
    result_p[0] = static_cast<unsigned char>((value >> 8) & 0xFF);
    result_p[1] = static_cast<unsigned char>(value & 0xFF);
    return result;
}

} // namespace million