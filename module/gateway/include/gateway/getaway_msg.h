#pragma once

#include <cassert>

#include <utility>

#include <million/imsg.h>
#include <million/proto_msg.h>

#include <gateway/api.h>
#include <gateway/user_session_handle.h>

namespace million {
namespace gateway {

// recv
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRegisterProtoMsg, (int32_t) msg_id, (std::vector<CommProtoSubMsgInfo>) sub_msg_list)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRegisterNoTokenServiceMsg, (ServiceHandle) service_handle, (uint32_t) msg_id)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRegisterServiceMsg, (ServiceHandle) service_handle, (uint32_t) msg_id)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySetTokenMsg, (UserSessionHandle) user_session_handle)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySendProtoMsg, (UserSessionHandle) user_session_handle, (ProtoMsgUnique) proto_msg)

// send
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRecvProtoMsg, (UserSessionHandle) user_session_handle, (ProtoMsgUnique) proto_msg)


template <typename MsgExtIdT, typename SubMsgExtIdT>
inline std::optional<MsgUnique> MakeRegisterProtoMsg(const protobuf::FileDescriptor& file_desc, MsgExtIdT msg_ext_id, SubMsgExtIdT sub_msg_ext_id) {
    uint32_t msg_id = 0;
    std::vector<CommProtoSubMsgInfo> sub_msg_list;
    if (!CommProtoGetMessageList(file_desc, msg_ext_id, sub_msg_ext_id, &msg_id, &sub_msg_list)) {
        return std::nullopt;
    }
    return std::make_unique<RegisterProtoMsg>(msg_id, std::move(sub_msg_list));
}

} // namespace gateway
} // namespace million
