#pragma once

#include <cassert>

#include <utility>

#include <protogen/cs/cs_msgid.pb.h>

#include <million/imsg.h>
#include <million/proto_msg.h>

#include <gateway/user_session_handle.h>

namespace million {
namespace gateway {

MILLION_MSG_DEFINE(RegisterServiceMsg, (ServiceHandle) service_handle, (Cs::MsgId) cs_msg_id)
// MILLION_MSG_DEFINE(UnRegisterServiceMsg, (ServiceHandle) service_handle, (Cs::MsgId) cs_msg_id)
MILLION_MSG_DEFINE(RecvProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)
MILLION_MSG_DEFINE(SendProtoMsg, (UserSessionHandle) session_handle, (ProtoMsgUnique) proto_msg)

} // namespace gateway
} // namespace million
