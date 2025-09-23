#include <million/proto_codec.h>

#include <million/proto_mgr.h>

namespace million {

std::optional<std::pair<ModuleId, ProtoMessageId>> ProtoCodec::FindMessageId(const protobuf::Message& message) const {
    auto key = FindMessageKey(message.GetDescriptor());
    if (!key) return std::nullopt;
    return DecodeModuleCode<ModuleId, ProtoMessageId>(*key);
}

std::optional<net::Packet> ProtoCodec::EncodeMessage(const protobuf::Message& message) const {
    auto desc = message.GetDescriptor();
    auto msg_key_opt = FindMessageKey(desc);
    if (!msg_key_opt) {
        return std::nullopt;
    }
    
    auto msg_key = host_to_network_u32(*msg_key_opt);
    auto packet = net::Packet(sizeof(msg_key) + message.ByteSize());
    size_t i = 0;
    std::memcpy(packet.data() + i, &msg_key, sizeof(msg_key));
    i += sizeof(msg_key);

    message.SerializeToArray(packet.data() + i, message.ByteSize());
    return packet;
}


std::optional<ProtoCodec::DecodeRes> ProtoCodec::DecodeMessage(net::PacketSpan packet) const {
    DecodeRes res = { 0 };

    if (packet.size() < sizeof(ProtoMessageKey)) return std::nullopt;

    ProtoMessageKey msg_key_net;

    size_t i = 0;
    std::memcpy(&msg_key_net, packet.data() + i, sizeof(msg_key_net));
    i += sizeof(msg_key_net);

    auto msg_key = host_to_network_u32(msg_key_net);

    auto [module_id, msg_id] = DecodeModuleCode<ModuleId, ProtoMessageId>(msg_key);
    res.module_id = module_id;
    res.msg_id = msg_id;

    auto msg_opt = NewMessage(res.module_id, res.msg_id);
    if (!msg_opt) {
        return std::nullopt;
    }
    res.msg = std::move(*msg_opt);

    auto success = res.msg->ParseFromArray(packet.data() + i, packet.size() - i);
    if (!success) {
        return std::nullopt;
    }
    return res;
}


const protobuf::Descriptor* ProtoCodec::FindMessageDesc(ModuleId module_id, ProtoMessageId msg_id) const {
    auto key = EncodeModuleCode(module_id, msg_id);
    auto iter = msg_desc_map_.find(key);
    if (iter == msg_desc_map_.end()) return nullptr;
    return iter->second;
}

std::optional<ProtoMessageKey> ProtoCodec::FindMessageKey(const protobuf::Descriptor* desc) const {
    auto iter = msg_id_map_.find(desc);
    if (iter == msg_id_map_.end()) return std::nullopt;
    return iter->second;
}

std::optional<ProtoMessageUnique> ProtoCodec::NewMessage(ModuleId module_id, ProtoMessageId msg_id) const {
    auto desc = FindMessageDesc(module_id, msg_id);
    if (!desc) {
        return std::nullopt;
    }
    auto msg = proto_mgr_.NewMessage(*desc);
    if (!msg) {
        return std::nullopt;
    }
    return msg;
}

uint32_t ProtoCodec::host_to_network_u32(uint32_t value) const {
    return network_to_host_u32(value);
}

uint32_t ProtoCodec::network_to_host_u32(uint32_t value) const {
    uint32_t result;
    uint8_t* result_p = reinterpret_cast<uint8_t*>(&result);
    uint8_t* value_p = reinterpret_cast<uint8_t*>(&value);
    result_p[0] = value_p[3];
    result_p[1] = value_p[2];
    result_p[2] = value_p[1];
    result_p[3] = value_p[0];
    return result;
}

} // namespace million