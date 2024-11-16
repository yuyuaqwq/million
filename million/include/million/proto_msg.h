#pragma once

#include <vector>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/compiler/importer.h>

#include <million/net/packet.h>
#include <million/imsg.h>
#include <million/noncopyable.h>

namespace million {

using ProtoMsgUnique = std::unique_ptr<google::protobuf::Message>;

#undef max
constexpr static inline uint32_t kMsgIdMax = std::numeric_limits<uint16_t>::max();
constexpr static inline uint32_t kSubMsgIdMax = std::numeric_limits<uint16_t>::max();

inline uint32_t CalcKey(uint32_t msg_id, uint32_t sub_msg_id) {
    assert(static_cast<uint32_t>(msg_id) <= kMsgIdMax);
    assert(sub_msg_id <= kSubMsgIdMax);
    return uint32_t(static_cast<uint32_t>(msg_id) << 16) | static_cast<uint16_t>(sub_msg_id);
}

inline std::pair<uint32_t, uint32_t> CalcMsgId(uint32_t key) {
    auto msg_id = key >> 16;
    auto sub_msg_id = key & 0xffff;
    return std::make_pair(msg_id, sub_msg_id);
}

namespace protobuf = google::protobuf;

class CommProtoMgr : noncopyable {
public:
    // 初始化
    void Init() {

    }

    // 注册协议
    template <typename MsgExtIdT, typename SubMsgExtIdT>
    bool RegisterProto(const protobuf::FileDescriptor& file_desc, uint32_t msg_id, MsgExtIdT msg_ext_id, SubMsgExtIdT sub_msg_ext_id) {
        int enum_count = file_desc.enum_type_count();
        for (int i = 0; i < enum_count; i++) {
            const protobuf::EnumDescriptor* enum_desc = file_desc.enum_type(i);
            if (!enum_desc) continue;
            auto& enum_opts = enum_desc->options();
            if (!enum_opts.HasExtension(msg_ext_id)) {
                continue;
            }

            auto msg_id = enum_opts.GetExtension(msg_ext_id);
            auto msg_id_u32 = static_cast<uint32_t>(msg_id);
            if (msg_id_u32 > kMsgIdMax) {
                // throw std::runtime_error(std::format("RegistrySubMsgId error: msg_id:{} > kMsgIdMax", msg_id_u32));
                return false;
            }

            int message_count = file_desc.message_type_count();
            for (int j = 0; j < message_count; j++) {
                const protobuf::Descriptor* desc = file_desc.message_type(j);
                auto& msg_opts = desc->options();
                if (!msg_opts.HasExtension(sub_msg_ext_id)) {
                    continue;
                }

                auto sub_msg_id = msg_opts.GetExtension(sub_msg_ext_id);
                auto sub_msg_id_u32 = static_cast<uint32_t>(sub_msg_id);
                if (sub_msg_id_u32 > kSubMsgIdMax) {
                    // throw std::runtime_error(std::format("RegistrySubMsgId error: msg_id:{}, sub_msg_id:{} > kMsgIdMax", msg_id_u32, sub_msg_id_u32));
                    return false;
                }

                static_assert(sizeof(msg_id) == sizeof(sub_msg_id), "");

                auto key = CalcKey(msg_id_u32, sub_msg_id_u32);
                msg_desc_map_.emplace(key, desc);
                msg_id_map_.emplace(desc, key);
            }
            return true;
        }
        return false;
    }

    // 编码消息
    std::optional<net::Packet> EncodeMessage(const protobuf::Message& message) {
        auto desc = message.GetDescriptor();
        auto msg_key = GetMsgKey(desc);
        if (!msg_key) {
            return {};
        }
        auto [msg_id, sub_msg_id] = CalcMsgId(*msg_key);

        net::Packet buffer;
        uint16_t msg_id_net = host_to_network_short(static_cast<uint16_t>(msg_id));
        uint16_t sub_msg_id_net = host_to_network_short(static_cast<uint16_t>(sub_msg_id));

        buffer.resize(sizeof(msg_id_net) + sizeof(sub_msg_id_net) + message.ByteSize());
        size_t i = 0;
        std::memcpy(buffer.data() + i, &msg_id_net, sizeof(msg_id_net));
        i += sizeof(msg_id_net);
        std::memcpy(buffer.data() + i, &sub_msg_id_net, sizeof(sub_msg_id_net));
        i += sizeof(sub_msg_id_net);

        message.SerializeToArray(buffer.data() + i, message.ByteSize());
        return buffer;
    }

    // 解码消息
    std::optional<std::tuple<ProtoMsgUnique, uint32_t, uint32_t>> DecodeMessage(const net::Packet& buffer) {
        uint16_t msg_id_net, sub_msg_id_net;
        if (buffer.size() < sizeof(msg_id_net) + sizeof(sub_msg_id_net)) return std::nullopt;

        size_t i = 0;
        std::memcpy(&msg_id_net, buffer.data() + i, sizeof(msg_id_net));
        i += sizeof(msg_id_net);
        std::memcpy(&sub_msg_id_net, buffer.data() + i, sizeof(sub_msg_id_net));
        i += sizeof(sub_msg_id_net);

        uint32_t msg_id = static_cast<uint32_t>(network_to_host_short(msg_id_net));
        uint32_t sub_msg_id = static_cast<uint32_t>(network_to_host_short(sub_msg_id_net));
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

    uint16_t host_to_network_short(uint16_t value)
    {
        uint16_t result;
        unsigned char* result_p = reinterpret_cast<unsigned char*>(&result);
        result_p[0] = static_cast<unsigned char>((value >> 8) & 0xFF);
        result_p[1] = static_cast<unsigned char>(value & 0xFF);
        return result;
    }

    uint16_t network_to_host_short(uint16_t value)
    {
        uint16_t result;
        unsigned char* result_p = reinterpret_cast<unsigned char*>(&result);
        result_p[0] = static_cast<unsigned char>((value >> 8) & 0xFF);
        result_p[1] = static_cast<unsigned char>(value & 0xFF);
        return result;
    }

private:
    std::unordered_map<uint32_t, const protobuf::Descriptor*> msg_desc_map_;
    std::unordered_map<const protobuf::Descriptor*, uint32_t> msg_id_map_;
};

#define MILLION_PROTO_MSG_DISPATCH(NAMESPACE_, HANDLE_TYPE_) \
    ::million::Task<> ProtoMsgDispatch(const HANDLE_TYPE_& handle, ::NAMESPACE_::MsgId msg_id, uint32_t sub_msg_id, ::million::ProtoMsgUnique&& proto_msg) { \
        auto iter = _MILLION_PROTO_MSG_HANDLE_MAP_.find(::million::CalcKey(msg_id, sub_msg_id)); \
        if (iter != _MILLION_PROTO_MSG_HANDLE_MAP_.end()) { \
            co_await (this->*iter->second)(handle, std::move(proto_msg)); \
        } \
        co_return; \
    } \
    ::NAMESPACE_::MsgId _MILLION_PROTO_MSG_HANDLE_CURRENT_MSG_ID_ = ::NAMESPACE_::MSG_ID_INVALID; \
    ::std::unordered_map<uint32_t, ::million::Task<>(_MILLION_SERVICE_TYPE_::*)(const HANDLE_TYPE_&, ::million::ProtoMsgUnique)> _MILLION_PROTO_MSG_HANDLE_MAP_ \

#define MILLION_PROTO_MSG_ID(NAMESPACE_, MSG_ID_) \
    const bool _MILLION_PROTO_MSG_HANDLE_SET_MSG_ID_##MSG_ID_ = \
        [this] { \
            _MILLION_PROTO_MSG_HANDLE_CURRENT_MSG_ID_ = ::NAMESPACE_::MSG_ID_; \
            return true; \
        }() \

#define MILLION_PROTO_MSG_HANDLE(NAMESPACE_, HANDLE_TYPE_, SUB_MSG_ID_, MSG_TYPE_, MSG_PTR_NAME_) \
    ::million::Task<> _MILLION_PROTO_MSG_HANDLE_##MSG_TYPE_##_I(const HANDLE_TYPE_& handle, ::million::ProtoMsgUnique MILLION_PROTO_MSG_) { \
        auto msg = ::std::unique_ptr<::NAMESPACE_::MSG_TYPE_>(static_cast<::NAMESPACE_::MSG_TYPE_*>(MILLION_PROTO_MSG_.release())); \
        co_await _MILLION_PROTO_MSG_HANDLE_##MSG_TYPE_##_II(handle, std::move(msg)); \
        co_return; \
    } \
    const bool MILLION_PROTO_MSG_HANDLE_REGISTER_##MSG_TYPE_ =  \
        [this] { \
            _MILLION_PROTO_MSG_HANDLE_MAP_.insert(::std::make_pair(::million::CalcKey(_MILLION_PROTO_MSG_HANDLE_CURRENT_MSG_ID_, NAMESPACE_::SUB_MSG_ID_), \
                &_MILLION_SERVICE_TYPE_::_MILLION_PROTO_MSG_HANDLE_##MSG_TYPE_##_I \
            )); \
            return true; \
        }(); \
    ::million::Task<> _MILLION_PROTO_MSG_HANDLE_##MSG_TYPE_##_II(const HANDLE_TYPE_& handle, ::std::unique_ptr<::NAMESPACE_::MSG_TYPE_> MSG_PTR_NAME_)


} // namespace million