#include <million/api.h>

#include <million/nonnull_ptr.h>
#include <million/imsg.h>
#include <million/iservice.h>

#include <gateway/gateway.h>

#include <agent/api.h>

namespace million {
namespace agent {

class AgentService;
using AgentLogicHandleFunc = Task<>(*)(AgentService* agent, ProtoMsgUnique proto_msg);

class MILLION_AGENT_API AgentLogicHandler {
public:
    static AgentLogicHandler& Instance() {
        static AgentLogicHandler handler;
        return handler;
    }

    void Init(nonnull_ptr<ProtoCodec> proto_codec) {
        proto_codec_ = proto_codec.get();
        for (auto& func : logic_init_queue_) {
            func();
        }
        logic_init_queue_.clear();
    }


    template <typename MsgExtIdT, typename SubMsgExtIdT>
    void RegisterLogicMsgProto(std::string proto_file_name, MsgExtIdT msg_ext_id, SubMsgExtIdT sub_msg_ext_id) {
        logic_init_queue_.emplace_back([this, proto_file_name = std::move(proto_file_name), msg_ext_id, sub_msg_ext_id] {
            if (!proto_codec_->RegisterProto(proto_file_name, msg_ext_id, sub_msg_ext_id)) {
                // 注册协议失败
            }
        });
    }

    void RegisterLogicMsgHandle(MsgKey msg_key, AgentLogicHandleFunc handle) {
        logic_init_queue_.emplace_back([this, msg_key, handle] {
            auto res = logic_handle_map_.emplace(msg_key, handle);
            if (!res.second) {
                // 重复注册消息
            }
        });
    }

    std::optional<AgentLogicHandleFunc> GetLogicHandle(MsgKey msg_key) {
        auto iter = logic_handle_map_.find(msg_key);
        if (iter == logic_handle_map_.end()) {
            return std::nullopt;
        }
        return iter->second;
    }

private:
    friend class AgentService;

    ProtoCodec* proto_codec_;

    std::unordered_map<MsgKey, AgentLogicHandleFunc> logic_handle_map_;
    std::vector<std::function<void()>> logic_init_queue_;
};


MILLION_MSG_DEFINE(MILLION_AGENT_API, AgentMgrLoginMsg, (gateway::UserContextId) context_id, (std::optional<ServiceHandle>) agent_handle);

class MILLION_AGENT_API AgentService : public IService {
public:
    using Base = IService;
    AgentService(IMillion* imillion, uint64_t user_context_id)
        : Base(imillion)
        , user_context_id_(user_context_id) {}

private:
    MILLION_MSG_DISPATCH(AgentService);

    using GatewayRecvPacketMsg = gateway::GatewayRecvPacketMsg;
    MILLION_MSG_HANDLE(GatewayRecvPacketMsg, msg) {
        auto res = AgentLogicHandler::Instance().proto_codec_->DecodeMessage(msg->packet);
        if (!res) {
            logger().Err("DecodeMessage failed");
            co_return;
        }
        auto func = AgentLogicHandler::Instance().GetLogicHandle(ProtoCodec::CalcKey(res->msg_id, res->sub_msg_id));
        if (!func) {
            logger().Err("Agent logic handle not found, msg_id:{}, sub_msg_id:{}", res->msg_id, res->sub_msg_id);
            co_return;
        }
        co_await(*func)(this, std::move(res->proto_msg));
        co_return;
    }

public:
    void SendToClient(const protobuf::Message& proto_msg) {
        auto res = AgentLogicHandler::Instance().proto_codec_->EncodeMessage(proto_msg);
        if (!res) {
            logger().Err("EncodeMessage failed: type:{}.", typeid(proto_msg).name());
            return;
        }
        Send<gateway::GatewaySendPacketMsg>(gateway_, user_context_id_, *res);
    }

    gateway::UserContextId user_context_id() const { return user_context_id_; }

private:
    ServiceHandle gateway_;
    gateway::UserContextId user_context_id_;
};

#define MILLION_AGENT_LOGIC_MSG_ID(NAMESPACE_, MSG_ID_, PROTO_FILE_NAME, MSG_EXT_ID_, SUB_MSG_EXT_ID_) \
    static ::million::MsgId _MILLION_AGENT_LOGIC_HANDLE_CURRENT_MSG_ID_ = 0; \
    const bool _MILLION_AGENT_LOGIC_HANDLE_SET_MSG_ID_##MSG_ID_ = \
        [] { \
            ::million::agent::AgentLogicHandler::Instance().RegisterLogicMsgProto(PROTO_FILE_NAME, NAMESPACE_::MSG_EXT_ID_, NAMESPACE_::SUB_MSG_EXT_ID_); \
            _MILLION_AGENT_LOGIC_HANDLE_CURRENT_MSG_ID_ = static_cast<::million::MsgId>(NAMESPACE_::MSG_ID_); \
            return true; \
        }() \

#define MILLION_AGENT_LOGIC_HANDLE(NAMESPACE_, SUB_MSG_ID_, MSG_TYPE_, MSG_PTR_NAME_) \
    ::million::Task<> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(::million::agent::AgentService* agent, ::std::unique_ptr<NAMESPACE_::MSG_TYPE_> MSG_NAME_); \
    ::million::Task<> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_I(::million::agent::AgentService* agent, ::million::ProtoMsgUnique MSG_PTR_NAME_) { \
        auto msg = ::std::unique_ptr<NAMESPACE_::MSG_TYPE_>(static_cast<NAMESPACE_::MSG_TYPE_*>(MSG_PTR_NAME_.release())); \
        co_await _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(agent, std::move(msg)); \
        co_return; \
    } \
    const bool MILLION_AGENT_LOGIC_HANDLE_REGISTER_##MSG_TYPE_ =  \
        [] { \
            ::million::agent::AgentLogicHandler::Instance().RegisterLogicMsgHandle(::million::ProtoCodec::CalcKey(_MILLION_AGENT_LOGIC_HANDLE_CURRENT_MSG_ID_, NAMESPACE_::SUB_MSG_ID_), \
                _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_I); \
            return true; \
        }(); \
    ::million::Task<> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(::million::agent::AgentService* agent, ::std::unique_ptr<NAMESPACE_::MSG_TYPE_> MSG_PTR_NAME_)


} // namespace agent
} // namespace million