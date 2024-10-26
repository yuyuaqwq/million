#pragma once

#include <cassert>

#include <utility>

#include <protogen/cs/cs_msgid.pb.h>

#include <million/imsg.h>

namespace million {
namespace gateway {

enum class GatewayMsgType : uint32_t {
    kMin = 1,

    kRegisterService = 2,
    // kUnRegisterService,
    kRecvProtoMsg = 4,
    kSendProtoMsg = 5,

    kMax = 1024,
};
using GatewayMsgBase = MsgBaseT<GatewayMsgType>;

MILLION_MSG_DEFINE(RegisterServiceMsg, GatewayMsgType::kRegisterService, (ServiceHandle) service_handle, (Cs::MsgId) cs_msg_id)
// MILLION_MSG_DEFINE(UnRegisterServiceMsg, kUnRegisterService, (ServiceHandle) service_handle, (Cs::MsgId) cs_msg_id)
MILLION_MSG_DEFINE(RecvProtoMsg, GatewayMsgType::kRecvProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)
MILLION_MSG_DEFINE(SendProtoMsg, GatewayMsgType::kSendProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)

} // namespace gateway
} // namespace million
