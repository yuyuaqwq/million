#pragma once

#include <vector>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/compiler/importer.h>

#include <asio.hpp>

#include <protogen/cs/cs_msgid.pb.h>

#include <million/proto_msg.h>

#include "token.h"

namespace million {

namespace protobuf = google::protobuf;

struct ProtoAdditionalHeader {
    Token token;
};

class ProtoMgr {
public:
    // 初始化消息ID
    void Init() {
        const protobuf::DescriptorPool* pool = protobuf::DescriptorPool::generated_pool();
        protobuf::DescriptorDatabase* db = pool->internal_generated_database();
        if (db == nullptr) {
            return;
        }

        std::vector<std::string> file_names;
        db->FindAllFileNames(&file_names);   // 遍历得到所有proto文件名
        for (const std::string& filename : file_names) {
            const protobuf::FileDescriptor* file_descriptor = pool->FindFileByName(filename);
            if (file_descriptor == nullptr) continue;

            // 获取该文件中定义的 SubMsgId Enum
            int enum_count = file_descriptor->enum_type_count();
            const protobuf::EnumDescriptor* enum_descriptor = nullptr;
            for (int i = 0; i < enum_count; i++) {
                const protobuf::EnumDescriptor* descriptor = file_descriptor->enum_type(i);
                if (!descriptor) continue;
                const std::string& name = descriptor->full_name();
                auto& opts = descriptor->options();
                if (opts.HasExtension(Cs::cs_msg_id)) {
                    Cs::MsgId msg_id = opts.GetExtension(Cs::cs_msg_id);

                    // 建立这个文件与对应的msg_id的映射
                    file_desc_map_.insert(std::make_pair(msg_id, file_descriptor));
                    break;
                }
            }
        }
    }

    // 注册子消息
    template <typename ExtensionIdT>
    bool Registry(Cs::MsgId msg_id, const ExtensionIdT& id) {
        if (static_cast<uint32_t>(msg_id) > kMsgIdMax) {
            throw std::runtime_error(std::format("RegistrySubMsgId error: msg_id:{} > kMsgIdMax", static_cast<uint32_t>(msg_id)));
        }
        auto desc = FindMsgDesc(msg_id);
        if (!desc) return false;
        auto& options = desc->options();
        if (options.HasExtension(id)) {
            auto sub_msg_id_t = options.GetExtension(id);
            auto sub_msg_id = static_cast<uint32_t>(sub_msg_id_t);
            if (sub_msg_id > kSubMsgIdMax) {
                throw std::runtime_error(std::format("RegistrySubMsgId error: msg_id:{}, sub_msg_id:{} > kMsgIdMax", static_cast<uint32_t>(msg_id), static_cast<uint32_t>(sub_msg_id)));
            }
            static_assert(sizeof(Cs::MsgId) == sizeof(sub_msg_id), "");
            auto key = CalcKey(msg_id, sub_msg_id);
            msg_desc_map_.insert({ key, desc });
            msg_id_map_.insert({ desc, key });
        }
        return true;
    }

    // 编码消息
    std::optional<Buffer> EncodeMessage(const ProtoAdditionalHeader& header, const protobuf::Message& message) {
        auto desc = message.GetDescriptor();
        auto msg_key = GetMsgKey(desc);
        if (!msg_key) {
            return {};
        }
        auto [msg_id, sub_msg_id] = CalcMsgId(*msg_key);

        Buffer buffer;
        // 添加 msg_id 和 sub_msg_id
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

        // 序列化消息到缓冲区
        message.SerializeToArray(buffer.data() + i, message.ByteSize());
        return buffer;
    }

    // 解码消息
    std::optional<std::tuple<ProtoMsgUnique, Cs::MsgId, uint32_t>> DecodeMessage(const Buffer& buffer, ProtoAdditionalHeader* header) {
        // 读取 msg_id 和 sub_msg_id
        uint16_t msg_id_net, sub_msg_id_net;
        // 确保缓冲区足够大
        if (buffer.size() < sizeof(msg_id_net) + sizeof(sub_msg_id_net) + sizeof(*header)) return std::nullopt;

        size_t i = 0;
        std::memcpy(&msg_id_net, buffer.data() + i, sizeof(msg_id_net));
        i += sizeof(msg_id_net);
        std::memcpy(&sub_msg_id_net, buffer.data() + i, sizeof(sub_msg_id_net));
        i += sizeof(sub_msg_id_net);
        if (header) { std::memcpy(header, buffer.data() + i, sizeof(*header)); }
        i += sizeof(*header);

        Cs::MsgId msg_id = static_cast<Cs::MsgId>(asio::detail::socket_ops::network_to_host_short(msg_id_net));
        uint32_t sub_msg_id = static_cast<uint32_t>(asio::detail::socket_ops::network_to_host_short(sub_msg_id_net));
        if (static_cast<uint32_t>(msg_id) > kMsgIdMax) {
            return std::nullopt;
        }
        if (sub_msg_id > kMsgIdMax) {
            return std::nullopt;
        }

        auto proto_msg = NewMessage(msg_id, sub_msg_id);
        // 反序列化消息
        auto success = proto_msg->ParseFromArray(buffer.data() + i, buffer.size() - i);
        if (!success) {
            return std::nullopt;
        }
        return std::make_tuple(std::move(proto_msg), msg_id, sub_msg_id);
    }

private:
    const protobuf::Descriptor* FindMsgDesc(Cs::MsgId msg_id) {
        auto iter = file_desc_map_.find(msg_id);
        if (iter == file_desc_map_.end()) { return nullptr; }
        const protobuf::FileDescriptor* file_descriptor = iter->second;

        int message_count = file_descriptor->message_type_count();
        for (int i = 0; i < message_count; i++) {
            const protobuf::Descriptor* desc = file_descriptor->message_type(i);
            return desc;
        }
        return nullptr;
    }

    const protobuf::Descriptor* GetMsgDesc(Cs::MsgId msg_id, uint32_t sub_msg_id) {
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

    ProtoMsgUnique NewMessage(Cs::MsgId msg_id, uint32_t sub_msg_id) {
        auto desc = GetMsgDesc(msg_id, sub_msg_id);
        const protobuf::Message* proto_msg = protobuf::MessageFactory::generated_factory()->GetPrototype(desc);
        if (proto_msg != nullptr)
        {
            return ProtoMsgUnique(proto_msg->New());
        }
        return nullptr;
    }

    uint32_t CalcKey(Cs::MsgId msg_id, uint32_t sub_msg_id) {
        static_assert(sizeof(Cs::MsgId) == sizeof(sub_msg_id), "");
        assert(static_cast<uint32_t>(msg_id) <= kMsgIdMax);
        assert(sub_msg_id <= kSubMsgIdMax);
        return uint32_t(static_cast<uint32_t>(msg_id) << 16) | static_cast<uint16_t>(sub_msg_id);
    }

    std::pair<Cs::MsgId, uint32_t> CalcMsgId(uint32_t key) {
        auto msg_id = static_cast<Cs::MsgId>(key >> 16);
        auto sub_msg_id = key & 0xffff;
        return std::make_pair(msg_id, sub_msg_id);
    }

private:
    static_assert(sizeof(Cs::MsgId) == sizeof(uint32_t), "sizeof(MsgId) error.");

    constexpr static inline uint32_t kMsgIdMax = std::numeric_limits<uint16_t>::max();
    constexpr static inline uint32_t kSubMsgIdMax = std::numeric_limits<uint16_t>::max();

    std::unordered_map<Cs::MsgId, const protobuf::FileDescriptor*> file_desc_map_;
    std::unordered_map<uint32_t, const protobuf::Descriptor*> msg_desc_map_;
    std::unordered_map<const protobuf::Descriptor*, uint32_t> msg_id_map_;
};

} // namespace million