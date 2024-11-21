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

// recv
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRegisterUserServiceMsg, (ServiceHandle)user_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySureAgentMsg, (uint64_t)session, (ServiceHandle)agent_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySendProtoMsg, (uint64_t)session, (ProtoMsgUnique)proto_msg)

// send
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRecvProtoMsg, (uint64_t)session, (ProtoMsgUnique)proto_msg)

} // namespace gateway
} // namespace million
