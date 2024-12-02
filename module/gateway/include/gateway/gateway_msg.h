#pragma once

#include <cassert>

#include <vector>
#include <unordered_map>
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

using UserContextId = uint64_t;

// recv
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRegisterUserServiceMsg, (ServiceHandle) user_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySureAgentMsg, (UserContextId) context_id, (ServiceHandle) agent_service)
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewaySendPacketMsg, (UserContextId) context_id, (net::Packet) packet)

// send
MILLION_MSG_DEFINE(GATEWAY_CLASS_API, GatewayRecvPacketMsg, (UserContextId) context_id, (net::Packet) packet_raw, (net::PacketSpan) packet)


MILLION_MSG_DEFINE(GATEWAY_CLASS_API, NodeMgrNewAgentMsg, (uint64_t) context_id, (std::optional<ServiceHandle>) agent_handle);

MILLION_MSG_DEFINE(GATEWAY_CLASS_API, AgentRecvProtoMsg, (ProtoMsgUnique) proto_msg);

MILLION_MSG_DEFINE(GATEWAY_CLASS_API, AgentMgrLoginMsg, (UserContextId) context_id, (std::optional<ServiceHandle>) agent_handle);

class AgentService;
using MsgLogicHandleFunc = Task<>(*)(AgentService* agent, const protobuf::Message& proto_msg);
extern GATEWAY_OBJECT_API ProtoCodec* g_agent_proto_codec;
extern GATEWAY_OBJECT_API std::unordered_map<MsgKey, MsgLogicHandleFunc>* g_agent_logic_handle_map;
extern GATEWAY_OBJECT_API std::vector<std::function<void()>>* g_agent_logic_init;
extern GATEWAY_OBJECT_API MsgId _MILLION_AGENT_LOGIC_HANDLE_CURRENT_MSG_ID_;


// 允许用户继承agentservice

#define MILLION_AGENT_LOGIC_MSG_ID(MSG_ID_, PROTO_FILE_NAME, MSG_EXT_ID_, SUB_MSG_EXT_ID_) \
    static million::MsgId _MILLION_AGENT_LOGIC_HANDLE_CURRENT_MSG_ID_ = 0; \
    const bool _MILLION_AGENT_LOGIC_HANDLE_SET_MSG_ID_ = \
        [] { \
            if (!::million::gateway::g_agent_logic_init) { ::million::gateway::g_agent_logic_init = new std::vector<std::function<void()>>(); } \
                ::million::gateway::g_agent_logic_init->emplace_back([] { \
                    const protobuf::DescriptorPool* pool = protobuf::DescriptorPool::generated_pool(); \
                    protobuf::DescriptorDatabase* db = pool->internal_generated_database(); \
                    auto file_desc = pool->FindFileByName(PROTO_FILE_NAME); \
                    if (file_desc) { \
                    ::million::gateway::g_agent_proto_codec->RegisterProto(*file_desc, MSG_EXT_ID_, SUB_MSG_EXT_ID_); \
                    } \
                }); \
            _MILLION_AGENT_LOGIC_HANDLE_CURRENT_MSG_ID_ = static_cast<::million::MsgId>(MSG_ID_); \
            return true; \
        }() \

#define MILLION_AGENT_LOGIC_HANDLE(NAMESPACE_, SUB_MSG_ID_, MSG_TYPE_, MSG_NAME_) \
    ::million::Task<> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(::million::gateway::AgentService* agent, const NAMESPACE_::MSG_TYPE_& MSG_NAME_); \
    ::million::Task<> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_I(::million::gateway::AgentService* agent, const protobuf::Message& MSG_NAME_) { \
        co_await _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(agent, *static_cast<const NAMESPACE_::MSG_TYPE_*>(&MSG_NAME_)); \
        co_return; \
    } \
    const bool MILLION_AGENT_LOGIC_HANDLE_REGISTER_##MSG_TYPE_ =  \
        [] { \
            if (!::million::gateway::g_agent_logic_init) { ::million::gateway::g_agent_logic_init = new std::vector<std::function<void()>>(); } \
            ::million::gateway::g_agent_logic_init->emplace_back([] { \
                auto res = ::million::gateway::g_agent_logic_handle_map->emplace(::million::ProtoCodec::CalcKey(_MILLION_AGENT_LOGIC_HANDLE_CURRENT_MSG_ID_, NAMESPACE_::SUB_MSG_ID_), \
                    _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_I \
                ); \
                assert(res.second); \
            }); \
            return true; \
        }(); \
    ::million::Task<> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(::million::gateway::AgentService* agent, const NAMESPACE_::MSG_TYPE_& MSG_NAME_)


//static MsgId s_msg_id = 0;
//Task<> MsgHandle(AgentService* agent, const protobuf::Message& proto_msg);
//const bool _MILLION_AGENT_MSG_HANDLE_REGISTER_ = [] { 
//    auto res = g_agent_logic_handle_map->emplace(g_agent_proto_codec->CalcKey(s_msg_id, 1), MsgHandle);
//    assert(res.second);
//    return true;
//}();
//Task<> MsgHandle(AgentService* agent, const protobuf::Message& proto_msg) {
//    AgentSend(agent, proto_msg);
//    co_return;
//}


} // namespace gateway
} // namespace million
