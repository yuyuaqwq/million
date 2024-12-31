#include <million/api.h>

#include <million/imillion.h>

#include <gateway/gateway.h>

#include <agent/api.h>

namespace million {
namespace agent {

MILLION_MSG_DEFINE(MILLION_AGENT_API, NewAgentMsg, (SessionId) user_session_id, (std::optional<ServiceHandle>) agent_handle);

class AgentService;
using AgentLogicHandleFunc = Task<>(*)(AgentService* agent, ProtoMsgUnique proto_msg);

class MILLION_AGENT_API AgentLogicHandler {
public:
    static AgentLogicHandler& Instance() {
        static AgentLogicHandler handler;
        return handler;
    }

    void Init(nonnull_ptr<IMillion> imillion, nonnull_ptr<ProtoCodec> proto_codec) {
        imillion_ = imillion.get();
        proto_codec_ = proto_codec.get();
        for (auto& func : logic_init_queue_) {
            func();
        }
        logic_init_queue_.clear();
    }


    template <typename MsgExtIdT, typename SubMsgExtIdT>
    void RegisterProto(std::string proto_file_name, MsgExtIdT msg_ext_id, SubMsgExtIdT sub_msg_ext_id) {
        logic_init_queue_.emplace_back([this, proto_file_name = std::move(proto_file_name), msg_ext_id, sub_msg_ext_id] {
            if (!proto_codec_->RegisterFile(proto_file_name, msg_ext_id, sub_msg_ext_id)) {
                // 注册协议失败
                imillion_->logger().Err("RegisterProto failed: {}.", proto_file_name);
            }
        });
    }

    void RegisterLogicHandle(MsgTypeKey msg_type_key, AgentLogicHandleFunc handle) {
        logic_init_queue_.emplace_back([this, msg_type_key, handle] {
            auto res = logic_handle_map_.emplace(msg_type_key, handle);
            if (!res.second) {
                // 重复注册消息处理函数
                imillion_->logger().Err("Duplicate registration processing handle: {}.", msg_type_key);
            }
        });
    }

    std::optional<AgentLogicHandleFunc> GetLogicHandle(MsgTypeKey msg_type_key) {
        auto iter = logic_handle_map_.find(msg_type_key);
        if (iter == logic_handle_map_.end()) {
            return std::nullopt;
        }
        return iter->second;
    }

private:
    friend class AgentService;

    IMillion* imillion_;
    ProtoCodec* proto_codec_;

    std::unordered_map<MsgTypeKey, AgentLogicHandleFunc> logic_handle_map_;
    std::vector<std::function<void()>> logic_init_queue_;
};

MILLION_MSG_DEFINE(MILLION_AGENT_API, AgentMgrLoginMsg, (SessionId) user_session_id, (std::optional<ServiceHandle>) agent_handle);

class MILLION_AGENT_API AgentService : public IService {
public:
    using Base = IService;
    AgentService(IMillion* imillion, uint64_t user_session_id)
        : Base(imillion)
        , user_session_id_(user_session_id) {}

private:
    MILLION_MSG_DISPATCH(AgentService);

    using GatewayRecvPacketMsg = gateway::GatewayRecvPacketMsg;
    MILLION_MSG_HANDLE(GatewayRecvPacketMsg, msg) {
        auto res = AgentLogicHandler::Instance().proto_codec_->DecodeMessage(msg->packet);
        if (!res) {
            logger().Err("DecodeMessage failed");
            co_return;
        }
        auto func = AgentLogicHandler::Instance().GetLogicHandle(res->msg.GetTypeKey());
        if (!func) {
            logger().Err("Agent logic handle not found, msg_id:{}, sub_msg_id:{}", res->msg_id, res->sub_msg_id);
            co_return;
        }
        co_await(*func)(this, std::move(res->msg.GetProtoMessage()));
        co_return;
    }

public:
    void SendToClient(const protobuf::Message& proto_msg) {
        auto res = AgentLogicHandler::Instance().proto_codec_->EncodeMessage(proto_msg);
        if (!res) {
            logger().Err("EncodeMessage failed: type:{}.", typeid(proto_msg).name());
            return;
        }
        Reply<gateway::GatewaySendPacketMsg>(gateway_, user_session_id_, *res);
    }

    SessionId agent_id() const { return user_session_id_; }

private:
    ServiceHandle gateway_;
    SessionId user_session_id_;
};

#define MILLION_AGENT_LOGIC_REGISTER_PROTO(PROTO_FILE_NAME, NAME, MSG_EXT_ID_, SUB_MSG_EXT_ID_) \
    const bool _MILLION_AGENT_LOGIC_HANDLE_REGISTER_PROTO_##MSG_ID_ = \
        [] { \
            ::million::agent::AgentLogicHandler::Instance().RegisterProto(PROTO_FILE_NAME, MSG_EXT_ID_, SUB_MSG_EXT_ID_); \
            return true; \
        }() \

#define MILLION_AGENT_LOGIC_HANDLE(MSG_TYPE_, MSG_PTR_NAME_) \
    ::million::Task<> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(::million::agent::AgentService* agent, ::std::unique_ptr<MSG_TYPE_> MSG_NAME_); \
    ::million::Task<> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_I(::million::agent::AgentService* agent, ::million::ProtoMsgUnique MSG_PTR_NAME_) { \
        auto msg = ::std::unique_ptr<MSG_TYPE_>(static_cast<MSG_TYPE_*>(MSG_PTR_NAME_.release())); \
        co_await _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(agent, std::move(msg)); \
        co_return; \
    } \
    const bool MILLION_AGENT_LOGIC_HANDLE_REGISTER_##MSG_TYPE_ =  \
        [] { \
            ::million::agent::AgentLogicHandler::Instance().RegisterLogicHandle(reinterpret_cast<::million::MsgTypeKey>(MSG_TYPE_::GetDescriptor()), \
                _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_I); \
            return true; \
        }(); \
    ::million::Task<> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(::million::agent::AgentService* agent, ::std::unique_ptr<MSG_TYPE_> MSG_PTR_NAME_)


} // namespace agent
} // namespace million