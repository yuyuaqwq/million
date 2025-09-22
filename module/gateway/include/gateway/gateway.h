#pragma once

#include <cassert>

#include <vector>
#include <unordered_map>
#include <utility>

#include <million/imillion.h>

#include <gateway/api.h>

namespace million {
namespace gateway {

// 注册user服务，没有token的消息发往user-n服务
// user-n服务再通知agentmgr(全局唯一)服务，agentmgr让nodemgr(本机唯一)创建agent-n，然后再关联到gateway，gateway下次就可以直接发给这个agent

// recv
MILLION_MESSAGE_DEFINE(MILLION_GATEWAY_API, GatewayRegisterUserServiceReq, (ServiceHandle) user_service);
MILLION_MESSAGE_DEFINE_EMPTY(MILLION_GATEWAY_API, GatewayRegisterUserServiceResp);

MILLION_MESSAGE_DEFINE(MILLION_GATEWAY_API, GatewaySureAgent, (ServiceHandle) agent_service);
MILLION_MESSAGE_DEFINE(MILLION_GATEWAY_API, GatewaySendPacket, (net::Packet) packet);

MILLION_MESSAGE_DEFINE(MILLION_GATEWAY_API, GatewayResetAgentId, (SessionId) agent_id);

// send
// MILLION_MESSAGE_DEFINE(MILLION_GATEWAY_API, GatewayRecvPacketReq, (net::Packet) packet_raw, (net::PacketSpan) packet);

} // namespace gateway
} // namespace million
