#pragma once

#include <cassert>

#include <utility>

#include <million/imsg.h>
#include <million/proto_msg.h>

#include <gateway/api.h>
#include <gateway/user_session_handle.h>

namespace million {
namespace gateway {

// ע��login����û��token����Ϣ����login-n����
// login-n������֪ͨagentmgr����agentmgr��nodemgr����agent-n��Ȼ���ٹ�����gateway��gateway�´ξͿ���ֱ�ӷ������agent

// recv
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRegisterLoginServiceMsg, (ServiceHandle) login_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySureAgentMsg, (UserSessionHandle) user_session, (ServiceHandle) agent_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySendPacketMsg, (UserSessionHandle) user_session, (net::Packet) packet)

// send
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRecvPacketMsg, (UserSessionHandle) user_session, (net::Packet) packet)

} // namespace gateway
} // namespace million
