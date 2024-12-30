#pragma once

#include <million/noncopyable.h>
#include <million/exception.h>
#include <million/msg.h>
#include <million/net/packet.h>
#include <million/proto_mgr.h>

namespace million {

using MsgKey = uint32_t;
using MsgId = uint16_t;
using SubMsgId = uint16_t;

class MILLION_API ProtoCodec : noncopyable {
public:
    ProtoCodec(const ProtoMgr& proto_mgr)
        : proto_mgr_(proto_mgr) {}

    // 注册协议
    template <typename MsgExtIdT, typename SubMsgExtIdT>
    bool RegisterProto(const std::string& proto_file_name, MsgExtIdT msg_ext_id, SubMsgExtIdT sub_msg_ext_id) {
        //std::vector<std::string> file_names;
        //proto_mgr_.desc_db().FindAllFileNames(&file_names);   // 遍历得到所有proto文件名
        //for (const std::string& filename : file_names) {
        //    const protobuf::FileDescriptor* file_desc = proto_mgr_.desc_pool().FindFileByName(filename);
        //}

        auto file_desc = proto_mgr_.desc_pool().FindFileByName(proto_file_name);
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

            const auto* msg = proto_mgr_.msg_factory().GetPrototype(desc);
            if (!msg) {
                continue;
            }

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

    // 编码消息
    std::optional<net::Packet> EncodeMessage(const protobuf::Message& message) {
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

    // 解码消息
    struct DecodeRes {
        MsgId msg_id;
        SubMsgId sub_msg_id;
        MsgUnique msg;
    };
    std::optional<DecodeRes> DecodeMessage(net::PacketSpan packet) {
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

    std::optional<std::pair<MsgId, SubMsgId>> GetMsgId(const protobuf::Message& message) {
        auto key = GetMsgKey(message.GetDescriptor());
        if (!key) return std::nullopt;
        return CalcMsgId(*key);
    }

#undef max
    constexpr static inline MsgId kMsgIdMax = std::numeric_limits<MsgId>::max();
    constexpr static inline SubMsgId kSubMsgIdMax = std::numeric_limits<SubMsgId>::max();

    static MsgKey CalcKey(MsgId msg_id, SubMsgId sub_msg_id) {
        assert(static_cast<MsgKey>(msg_id) <= kMsgIdMax);
        assert(sub_msg_id <= kSubMsgIdMax);
        return MsgKey(static_cast<MsgKey>(msg_id) << sizeof(sub_msg_id) * 8) | static_cast<MsgKey>(sub_msg_id);
    }

    static std::pair<MsgId, SubMsgId> CalcMsgId(MsgKey key) {
        auto msg_id = key >> 16;
        auto sub_msg_id = key & 0xffff;
        return std::make_pair(msg_id, sub_msg_id);
    }

private:
    const protobuf::Descriptor* GetMsgDesc(MsgId msg_id, SubMsgId sub_msg_id) {
        auto key = CalcKey(msg_id, sub_msg_id);
        auto iter = msg_desc_map_.find(key);
        if (iter == msg_desc_map_.end()) return nullptr;
        return iter->second;
    }

    std::optional<MsgKey> GetMsgKey(const protobuf::Descriptor* desc) {
        auto iter = msg_id_map_.find(desc);
        if (iter == msg_id_map_.end()) return std::nullopt;
        return iter->second;
    }

    std::optional<MsgUnique> NewMessage(MsgId msg_id, SubMsgId sub_msg_id) {
        auto desc = GetMsgDesc(msg_id, sub_msg_id);
        if (!desc) return std::nullopt;
        const ProtoMessage* msg = proto_mgr_.msg_factory().GetPrototype(desc);
        if (msg != nullptr) {
            return MsgUnique(msg->New());
        }
        return std::nullopt;
    }

    uint16_t host_to_network_short(uint16_t value) {
        uint16_t result;
        unsigned char* result_p = reinterpret_cast<unsigned char*>(&result);
        result_p[0] = static_cast<unsigned char>((value >> 8) & 0xFF);
        result_p[1] = static_cast<unsigned char>(value & 0xFF);
        return result;
    }

    uint16_t network_to_host_short(uint16_t value) {
        uint16_t result;
        unsigned char* result_p = reinterpret_cast<unsigned char*>(&result);
        result_p[0] = static_cast<unsigned char>((value >> 8) & 0xFF);
        result_p[1] = static_cast<unsigned char>(value & 0xFF);
        return result;
    }

private:
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

#define MILLION_PROTO_PACKET_DISPATCH(PROTO_CODEC_, PROTO_PACKET_MSG_TYPE_) \
    using _MILLION_PROTO_PACKET_MSG_TYPE_ = PROTO_PACKET_MSG_TYPE_; \
    MILLION_MSG_HANDLE(PROTO_PACKET_MSG_TYPE_, msg) { \
        auto res = (PROTO_CODEC_)->DecodeMessage(msg->packet); \
        if (!res) { \
            co_return; \
        } \
        co_await OnMsg(sender, session_id, std::move(res->msg)); \
        co_return; \
    } \

} // namespace million