#pragma once

#include <cassert>

#include <utility>

#include <protogen/cs/cs_msgid.pb.h>

namespace million {
namespace gateway {

constexpr static inline uint32_t kMsgIdMax = std::numeric_limits<uint16_t>::max();
constexpr static inline uint32_t kSubMsgIdMax = std::numeric_limits<uint16_t>::max();

inline uint32_t CalcKey(Cs::MsgId msg_id, uint32_t sub_msg_id) {
    static_assert(sizeof(Cs::MsgId) == sizeof(sub_msg_id), "");
    assert(static_cast<uint32_t>(msg_id) <= kMsgIdMax);
    assert(sub_msg_id <= kSubMsgIdMax);
    return uint32_t(static_cast<uint32_t>(msg_id) << 16) | static_cast<uint16_t>(sub_msg_id);
}

inline std::pair<Cs::MsgId, uint32_t> CalcMsgId(uint32_t key) {
    auto msg_id = static_cast<Cs::MsgId>(key >> 16);
    auto sub_msg_id = key & 0xffff;
    return std::make_pair(msg_id, sub_msg_id);
}
    
#define MILLION_PROTO_MSG_DISPATCH() \
    ::million::Task ProtoMsgDispatch(::million::gateway::UserSessionHandle handle, Cs::MsgId msg_id, uint32_t sub_msg_id, ::million::ProtoMsgUnique&& proto_msg) { \
        auto iter = _MILLION_PROTO_MSG_HANDLE_MAP_.find(::million::gateway::CalcKey(msg_id, sub_msg_id)); \
        if (iter != _MILLION_PROTO_MSG_HANDLE_MAP_.end()) { \
            co_await (this->*iter->second)(handle, std::move(proto_msg)); \
        } \
        co_return; \
    } \
    Cs::MsgId _MILLION_PROTO_MSG_HANDLE_CURRENT_MSG_ID_ = Cs::MSG_ID_INVALID; \
    ::std::unordered_map<uint32_t, ::million::Task(_MILLION_SERVICE_TYPE_::*)(::million::gateway::UserSessionHandle, ::million::ProtoMsgUnique)> _MILLION_PROTO_MSG_HANDLE_MAP_ \

#define MILLION_PROTO_MSG_ID(MSG_ID_) \
    const bool _MILLION_PROTO_MSG_HANDLE_SET_MSG_ID_##MSG_ID_ = \
        [this] { \
            _MILLION_PROTO_MSG_HANDLE_CURRENT_MSG_ID_ = ::Cs::MSG_ID_; \
            return true; \
        }() \

#define MILLION_PROTO_MSG_HANDLE(SUB_MSG_ID_, MSG_TYPE_, MSG_PTR_NAME_) \
    ::million::Task _MILLION_PROTO_MSG_HANDLE_##MSG_TYPE_##_I(::million::gateway::UserSessionHandle handle, ::million::ProtoMsgUnique MILLION_PROTO_MSG_) { \
        auto msg = ::std::unique_ptr<::Cs::MSG_TYPE_>(static_cast<::Cs::MSG_TYPE_*>(MILLION_PROTO_MSG_.release())); \
        co_await _MILLION_PROTO_MSG_HANDLE_##MSG_TYPE_##_II(handle, std::move(msg)); \
        co_return; \
    } \
    const bool MILLION_PROTO_MSG_HANDLE_REGISTER_##MSG_TYPE_ =  \
        [this] { \
            _MILLION_PROTO_MSG_HANDLE_MAP_.insert(::std::make_pair(::million::gateway::CalcKey(_MILLION_PROTO_MSG_HANDLE_CURRENT_MSG_ID_, Cs::SUB_MSG_ID_), \
                &_MILLION_SERVICE_TYPE_::_MILLION_PROTO_MSG_HANDLE_##MSG_TYPE_##_I \
            )); \
            return true; \
        }(); \
    ::million::Task _MILLION_PROTO_MSG_HANDLE_##MSG_TYPE_##_II(::million::gateway::UserSessionHandle handle, ::std::unique_ptr<::Cs::MSG_TYPE_> MSG_PTR_NAME_)

} // namespace gateway
} // namespace million
