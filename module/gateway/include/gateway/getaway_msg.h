#pragma once

#include <cassert>

#include <utility>

#include <million/imsg.h>
#include <million/proto_msg.h>

#include <gateway/api.h>
#include <gateway/user_session_handle.h>

namespace million {
namespace gateway {

// 注册login服务，没有token的消息发往login-n服务
// login-n服务再通知agentmgr服务，agentmgr让nodemgr创建agent-n，然后再关联到gateway，gateway下次就可以直接发给这个agent

// recv
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRegisterLoginServiceMsg, (ServiceHandle) login_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySureAgentMsg, (UserSessionHandle) user_session, (ServiceHandle) agent_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySendPacketMsg, (UserSessionHandle) user_session, (net::Packet) packet)

// send
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRecvPacketMsg, (UserSessionHandle) user_session, (net::Packet) packet)

} // namespace gateway
} // namespace million
