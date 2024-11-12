#pragma once

#include <vector>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>

#include <asio.hpp>

#include <million/noncopyable.h>
#include <million/proto_msg.h>
#include <million/net/packet.h>

#include <gateway/proto_msg.h>

namespace million {
namespace gateway {

namespace protobuf = google::protobuf;

template<typename HeaderT>
class CsProtoMgr : noncopyable {
public:
    // 初始化
    void Init() {

    }

    // 注册协议
    template <typename IdTagT, typename SubIdTagT>
    bool RegisterProto(const protobuf::FileDescriptor& file_desc, IdTagT id_tag, const SubIdTagT& sub_id_tag) {
        int enum_count = file_desc.enum_type_count();
        for (int i = 0; i < enum_count; i++) {
            const protobuf::EnumDescriptor* enum_desc = file_desc.enum_type(i);
            if (!enum_desc) continue;
            auto& enum_opts = enum_desc->options();
            if (!enum_opts.HasExtension(id_tag)) { // Cs::cs_msg_id
                continue;
            }
            auto msg_id = enum_opts.GetExtension(id_tag);
            auto msg_id_u32 = static_cast<uint32_t>(msg_id);
            if (msg_id_u32 > kMsgIdMax) {
                throw std::runtime_error(std::format("RegistrySubMsgId error: msg_id:{} > kMsgIdMax", msg_id_u32));
            }
            int message_count = file_desc.message_type_count();
            for (int i = 0; i < message_count; i++) {
                const protobuf::Descriptor* desc = file_desc.message_type(i);
                auto& msg_opts = desc->options();
                if (!msg_opts.HasExtension(sub_id_tag)) {
                    continue;
                }
                auto sub_msg_id = msg_opts.GetExtension(sub_id_tag);
                auto sub_msg_id_u32 = static_cast<uint32_t>(sub_msg_id);
                if (sub_msg_id_u32 > kSubMsgIdMax) {
                    throw std::runtime_error(std::format("RegistrySubMsgId error: msg_id:{}, sub_msg_id:{} > kMsgIdMax", msg_id_u32, sub_msg_id_u32));
                }
                static_assert(sizeof(msg_id) == sizeof(sub_msg_id), "");
                auto key = CalcKey(msg_id, sub_msg_id_u32);
                msg_desc_map_.insert({ key, desc });
                msg_id_map_.insert({ desc, key });
            }
            return true;
        }
        return false;
    }

    // 编码消息
    std::optional<net::Packet> EncodeMessage(const HeaderT& header, const protobuf::Message& message) {
        auto desc = message.GetDescriptor();
        auto msg_key = GetMsgKey(desc);
        if (!msg_key) {
            return {};
        }
        auto [msg_id, sub_msg_id] = CalcMsgId(*msg_key);

        net::Packet buffer;
        uint16_t msg_id_net = asio::detail::socket_ops::host_to_network_short(static_cast<uint16_t>(msg_id));
        uint16_t sub_msg_id_net = asio::detail::socket_ops::host_to_network_short(static_cast<uint16_t>(sub_msg_id));

        buffer.resize(sizeof(msg_id_net) + sizeof(sub_msg_id_net) + sizeof(header) + message.ByteSize());
        size_t i = 0;
        std::memcpy(buffer.data() + i, &msg_id_net, sizeof(msg_id_net));
        i += sizeof(msg_id_net);
        std::memcpy(buffer.data() + i, &sub_msg_id_net, sizeof(sub_msg_id_net));
        i += sizeof(sub_msg_id_net);
        std::memcpy(buffer.data() + i, &header, sizeof(header));
        i += sizeof(header);

        message.SerializeToArray(buffer.data() + i, message.ByteSize());
        return buffer;
    }

    // 解码消息
    std::optional<std::tuple<ProtoMsgUnique, uint32_t, uint32_t>> DecodeMessage(const net::Packet& buffer, HeaderT* header) {
        uint16_t msg_id_net, sub_msg_id_net;
        if (buffer.size() < sizeof(msg_id_net) + sizeof(sub_msg_id_net) + sizeof(*header)) return std::nullopt;

        size_t i = 0;
        std::memcpy(&msg_id_net, buffer.data() + i, sizeof(msg_id_net));
        i += sizeof(msg_id_net);
        std::memcpy(&sub_msg_id_net, buffer.data() + i, sizeof(sub_msg_id_net));
        i += sizeof(sub_msg_id_net);
        if (header) { std::memcpy(header, buffer.data() + i, sizeof(*header)); }
        i += sizeof(*header);

        uint32_t msg_id = static_cast<uint32_t>(asio::detail::socket_ops::network_to_host_short(msg_id_net));
        uint32_t sub_msg_id = static_cast<uint32_t>(asio::detail::socket_ops::network_to_host_short(sub_msg_id_net));
        if (msg_id > kMsgIdMax) {
            return std::nullopt;
        }
        if (sub_msg_id > kMsgIdMax) {
            return std::nullopt;
        }

        auto proto_msg_opt = NewMessage(msg_id, sub_msg_id);
        if (!proto_msg_opt) return {};
        auto& proto_msg = *proto_msg_opt;

        auto success = proto_msg->ParseFromArray(buffer.data() + i, buffer.size() - i);
        if (!success) {
            return std::nullopt;
        }
        return std::make_tuple(std::move(proto_msg), msg_id, sub_msg_id);
    }

private:
    const protobuf::Descriptor* GetMsgDesc(uint32_t msg_id, uint32_t sub_msg_id) {
        auto key = CalcKey(msg_id, sub_msg_id);
        auto iter = msg_desc_map_.find(key);
        if (iter == msg_desc_map_.end()) return nullptr;
        return iter->second;
    }

    std::optional<uint32_t> GetMsgKey(const protobuf::Descriptor* desc) {
        auto iter = msg_id_map_.find(desc);
        if (iter == msg_id_map_.end()) return std::nullopt;
        return iter->second;
    }

    std::optional<ProtoMsgUnique> NewMessage(uint32_t msg_id, uint32_t sub_msg_id) {
        auto desc = GetMsgDesc(msg_id, sub_msg_id);
        if (!desc) return std::nullopt;
        const protobuf::Message* proto_msg = protobuf::MessageFactory::generated_factory()->GetPrototype(desc);
        if (proto_msg != nullptr) {
            return ProtoMsgUnique(proto_msg->New());
        }
        return std::nullopt;
    }

private:
    std::unordered_map<uint32_t, const protobuf::Descriptor*> msg_desc_map_;
    std::unordered_map<const protobuf::Descriptor*, uint32_t> msg_id_map_;
};

} // namespace gateway
} // namespace million