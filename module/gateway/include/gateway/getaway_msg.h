#pragma once

#include <cassert>

#include <utility>

#include <million/imsg.h>

namespace million {
namespace gateway {

enum GatewayMsgType {
    kConnection,
    kRecvPacket,

    kRegisterService,
    // kUnRegisterService,
    kRecvProtoMsg,
    kSendProtoMsg,
};
using GatewayMsgBase = MsgBaseT<GatewayMsgType>;

MILLION_MSG_DEFINE(RegisterServiceMsg, kRegisterService, (ServiceHandle) service_handle, (Cs::MsgId) cs_msg_id)
// MILLION_MSG_DEFINE(UnRegisterServiceMsg, kUnRegisterService, (ServiceHandle) service_handle, (Cs::MsgId) cs_msg_id)
MILLION_MSG_DEFINE(RecvProtoMsg, kRecvProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)
MILLION_MSG_DEFINE(SendProtoMsg, kSendProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)

} // namespace gateway
} // namespace million
