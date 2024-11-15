#pragma once

#include <cassert>

#include <utility>

#include <million/imsg.h>
#include <million/proto_msg.h>

#include <gateway/api.h>
#include <gateway/user_session_handle.h>

namespace million {
namespace gateway {

	
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, RegisterProtoMsg, (const google::protobuf::FileDescriptor*) file_desc)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, RegisterNoTokenServiceMsg, (ServiceHandle) service_handle, (uint32_t) msg_id)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, RegisterServiceMsg, (ServiceHandle) service_handle, (uint32_t) msg_id)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, SetTokenMsg, (UserSessionHandle) user_session_handle)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, RecvProtoMsg, (UserSessionHandle) user_session_handle, (ProtoMsgUnique) proto_msg)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, SendProtoMsg, (UserSessionHandle) user_session_handle, (ProtoMsgUnique) proto_msg)

} // namespace gateway
} // namespace million
