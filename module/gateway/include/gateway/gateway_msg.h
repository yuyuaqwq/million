#pragma once

#include <cassert>

#include <utility>

#include <million/imsg.h>
#include <million/proto_msg.h>

#include <gateway/api.h>
#include <gateway/user_session_handle.h>

namespace million {
namespace gateway {

// ע��user����û��token����Ϣ����user-n����
// user-n������֪ͨagentmgr(ȫ��Ψһ)����agentmgr��nodemgr(����Ψһ)����agent-n��Ȼ���ٹ�����gateway��gateway�´ξͿ���ֱ�ӷ������agent

using GatewayContextId = uint64_t;

// recv
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRegisterUserServiceMsg, (ServiceHandle)user_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySureAgentMsg, (GatewayContextId)context_id, (ServiceHandle)agent_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySendPacketMsg, (GatewayContextId)context_id, (net::Packet)packet)

// send
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRecvPacketMsg, (GatewayContextId)context_id, (net::Packet)packet_raw, (net::PacketSpan)packet)

} // namespace gateway
} // namespace million
