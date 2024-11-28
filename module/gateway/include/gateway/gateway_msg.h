#pragma once

#include <cassert>

#include <utility>

#include <million/imsg.h>
#include <million/proto_msg.h>

#include <gateway/api.h>
#include <gateway/user_session_handle.h>

namespace million {
namespace gateway {

// 注册user服务，没有token的消息发往user-n服务
// user-n服务再通知agentmgr(全局唯一)服务，agentmgr让nodemgr(本机唯一)创建agent-n，然后再关联到gateway，gateway下次就可以直接发给这个agent

using GatewayContextId = uint64_t;

// recv
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRegisterUserServiceMsg, (ServiceHandle)user_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySureAgentMsg, (GatewayContextId)context_id, (ServiceHandle)agent_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySendPacketMsg, (GatewayContextId)context_id, (net::Packet)packet)

// send
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRecvPacketMsg, (GatewayContextId)context_id, (net::Packet)packet_raw, (net::PacketSpan)packet)

} // namespace gateway
} // namespace million
