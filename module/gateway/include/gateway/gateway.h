#pragma once

#include <cassert>

#include <vector>
#include <unordered_map>
#include <utility>

#include <million/msg.h>
#include <million/net/packet.h>
#include <million/service_handle.h>

#include <gateway/api.h>

namespace million {
namespace gateway {

// 注册user服务，没有token的消息发往user-n服务
// user-n服务再通知agentmgr(全局唯一)服务，agentmgr让nodemgr(本机唯一)创建agent-n，然后再关联到gateway，gateway下次就可以直接发给这个agent

// recv
MILLION_MSG_DEFINE(MILLION_GATEWAY_API, GatewayRegisterUserServiceMsg, (ServiceHandle) user_service);
MILLION_MSG_DEFINE(MILLION_GATEWAY_API, GatewaySureAgentMsg, (ServiceHandle) agent_service);
MILLION_MSG_DEFINE(MILLION_GATEWAY_API, GatewaySendPacketMsg, (net::Packet) packet);

// send
MILLION_MSG_DEFINE(MILLION_GATEWAY_API, GatewayRecvPacketMsg, (net::Packet) packet_raw, (net::PacketSpan) packet);

} // namespace gateway
} // namespace million
