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

MILLION_MSG_DEFINE(GATEWAY_CLASS_API, AgentMgrLoginMsg, (UserContextId) context_id, (std::optional<ServiceHandle>) agent_handle);

class AgentService;
using MsgLogicHandleFunc = Task<>(*)(AgentService* agent, ProtoMsgUnique proto_msg);

class AgentLogicHandler {
public:
    static AgentLogicHandler& Instance() {
        static AgentLogicHandler handler;
        return handler;
    }

    template <typename MsgExtIdT, typename SubMsgExtIdT>
    void RegisterLogicMsgProto(std::string proto_file_name, MsgExtIdT msg_ext_id, SubMsgExtIdT sub_msg_ext_id) {
        logic_init_queue_.emplace_back([this, proto_file_name = std::move(proto_file_name), msg_ext_id, sub_msg_ext_id] {
            const protobuf::DescriptorPool* pool = protobuf::DescriptorPool::generated_pool();
            protobuf::DescriptorDatabase* db = pool->internal_generated_database();
            auto file_desc = pool->FindFileByName(proto_file_name);
            if (!file_desc) {
                // 找不到该proto file
                return;
            }
            proto_codec_.RegisterProto(*file_desc, msg_ext_id, sub_msg_ext_id);
        });
    }

    void RegisterLogicMsgHandle(MsgKey msg_key, MsgLogicHandleFunc handle) {
        logic_init_queue_.emplace_back([this, msg_key, handle] {
            auto res = logic_handle_map_.emplace(msg_key, handle);
            if (res.second) {
                // 重复注册
            }
        });
    }

    void ExecInitLogicQueue() {
        for (auto& func : logic_init_queue_) {
            func();
        }
        logic_init_queue_.clear();
    }

    std::optional<MsgLogicHandleFunc> GetLogicHandle(MsgKey msg_key) {
        auto iter = logic_handle_map_.find(msg_key);
        if (iter == logic_handle_map_.end()) {
            return std::nullopt;
        }
        return iter->second;
    }

private:
    friend class AgentService;

    ProtoCodec proto_codec_;
    std::unordered_map<MsgKey, MsgLogicHandleFunc> logic_handle_map_;
    std::vector<std::function<void()>> logic_init_queue_;
};

#define MILLION_AGENT_LOGIC_MSG_ID(NAMESPACE_, MSG_ID_, PROTO_FILE_NAME, MSG_EXT_ID_, SUB_MSG_EXT_ID_) \
    static ::million::MsgId _MILLION_AGENT_LOGIC_HANDLE_CURRENT_MSG_ID_ = 0; \
    const bool _MILLION_AGENT_LOGIC_HANDLE_SET_MSG_ID_##MSG_ID_ = \
        [] { \
            ::million::gateway::AgentLogicHandler::Instance().RegisterLogicMsgProto(PROTO_FILE_NAME, NAMESPACE_::MSG_EXT_ID_, NAMESPACE_::SUB_MSG_EXT_ID_); \
            _MILLION_AGENT_LOGIC_HANDLE_CURRENT_MSG_ID_ = static_cast<::million::MsgId>(NAMESPACE_::MSG_ID_); \
            return true; \
        }() \

#define MILLION_AGENT_LOGIC_HANDLE(NAMESPACE_, SUB_MSG_ID_, MSG_TYPE_, MSG_PTR_NAME_) \
    ::million::Task<> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(::million::gateway::AgentService* agent, ::std::unique_ptr<NAMESPACE_::MSG_TYPE_> MSG_NAME_); \
    ::million::Task<> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_I(::million::gateway::AgentService* agent, ::million::ProtoMsgUnique MSG_PTR_NAME_) { \
        auto msg = ::std::unique_ptr<NAMESPACE_::MSG_TYPE_>(static_cast<NAMESPACE_::MSG_TYPE_*>(MSG_PTR_NAME_.release())); \
        co_await _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(agent, std::move(msg)); \
        co_return; \
    } \
    const bool MILLION_AGENT_LOGIC_HANDLE_REGISTER_##MSG_TYPE_ =  \
        [] { \
            ::million::gateway::AgentLogicHandler::Instance().RegisterLogicMsgHandle(::million::ProtoCodec::CalcKey(_MILLION_AGENT_LOGIC_HANDLE_CURRENT_MSG_ID_, NAMESPACE_::SUB_MSG_ID_), \
                _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_I); \
            return true; \
        }(); \
    ::million::Task<> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(::million::gateway::AgentService* agent, ::std::unique_ptr<NAMESPACE_::MSG_TYPE_> MSG_PTR_NAME_)

} // namespace gateway
} // namespace million
