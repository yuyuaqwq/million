#pragma once

#include <cassert>

#include <utility>

#include <million/imsg.h>
#include <million/proto_msg.h>
#include <million/iservice.h>

#include <gateway/api.h>
#include <gateway/user_session_handle.h>

namespace million {

namespace gateway {

// 注册user服务，没有token的消息发往user-n服务
// user-n服务再通知agentmgr(全局唯一)服务，agentmgr让nodemgr(本机唯一)创建agent-n，然后再关联到gateway，gateway下次就可以直接发给这个agent

using GatewayContextId = uint64_t;

// recv
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRegisterUserServiceMsg, (ServiceHandle) user_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySureAgentMsg, (GatewayContextId) context_id, (ServiceHandle) agent_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySendPacketMsg, (GatewayContextId) context_id, (net::Packet) packet)

// send
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRecvPacketMsg, (GatewayContextId) context_id, (net::Packet) packet_raw, (net::PacketSpan) packet)


MILLION_MSG_DEFINE(GATEWAY_CLASS_API, NodeMgrNewAgentMsg, (uint64_t) context_id, (std::optional<ServiceHandle>) agent_handle);

MILLION_MSG_DEFINE(GATEWAY_CLASS_API, AgentRecvProtoMsg, (ProtoMsgUnique) proto_msg);

MILLION_MSG_DEFINE(GATEWAY_CLASS_API, AgentMgrLoginMsg, (GatewayContextId) context_id, (std::optional<ServiceHandle>) agent_handle);

class AgentService;
using MsgLogicHandleFunc = Task<>(*)(AgentService* agent, const protobuf::Message& proto_msg);
extern GATEWAY_OBJECT_API ProtoCodec* g_agent_proto_codec;
extern GATEWAY_OBJECT_API std::unordered_map<MsgKey, MsgLogicHandleFunc>* g_agent_logic_handle_map;

void AgentSend(AgentService* agent, const protobuf::Message& proto_msg);

//static MsgId s_msg_id = 0;
//Task<> MsgHandle(AgentService* agent, const protobuf::Message& proto_msg);
//const bool _MILLION_AGENT_MSG_HANDLE_REGISTER_ = [] { 
//    auto res = g_agent_logic_handle_map->emplace(g_agent_proto_codec->CalcKey(s_msg_id, 1), MsgHandle);
//    assert(res.second);
//    return true;
//}();
//Task<> MsgHandle(AgentService* agent, const protobuf::Message& proto_msg) {
//    SendProtoMsg(agent, proto_msg);
//    co_return;
//}


} // namespace gateway
} // namespace million
