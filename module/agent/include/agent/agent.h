#include <million/api.h>

#include <million/imillion.h>

#include <gateway/gateway.h>

#include <agent/api.h>

namespace million {
namespace agent {

constexpr const char* kAgentServiceName = "AgentService";
constexpr const char* kAgentMgrServiceName = "AgentMgrService";

MILLION_MSG_DEFINE(MILLION_AGENT_API, NewAgentReq, (SessionId) user_session_id);
MILLION_MSG_DEFINE(MILLION_AGENT_API, NewAgentResp, (std::optional<ServiceHandle>) agent_handle);

class AgentService;
using AgentLogicHandleFunc = Task<MsgPtr>(*)(AgentService* agent, MsgPtr msg_ptr);

class MILLION_AGENT_API AgentLogicHandler {
public:
    static AgentLogicHandler& Instance() {
        static AgentLogicHandler handler;
        return handler;
    }

    void Init(IMillion* imillion) {
        imillion_ = imillion;
        for (auto& func : logic_init_queue_) {
            func();
        }
        logic_init_queue_.clear();
    }


    template <typename MsgExtIdT, typename SubMsgExtIdT>
    void RegisterProto(std::string proto_file_name, MsgExtIdT msg_ext_id, SubMsgExtIdT sub_msg_ext_id) {
        logic_init_queue_.emplace_back([this, proto_file_name = std::move(proto_file_name), msg_ext_id, sub_msg_ext_id] {
            if (!imillion_->proto_mgr().codec().RegisterFile(proto_file_name, msg_ext_id, sub_msg_ext_id)) {
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

    std::unordered_map<MsgTypeKey, AgentLogicHandleFunc> logic_handle_map_;
    std::vector<std::function<void()>> logic_init_queue_;
};

MILLION_MSG_DEFINE(MILLION_AGENT_API, AgentMgrLoginReq, (SessionId) agent_id);
MILLION_MSG_DEFINE(MILLION_AGENT_API, AgentMgrLoginResp, (std::optional<ServiceHandle>) agent_handle);


class MILLION_AGENT_API AgentService : public IService {
public:
    using Base = IService;
    AgentService(IMillion* imillion, uint64_t agent_id)
        : Base(imillion)
        , agent_id_(agent_id) {}

private:
    virtual Task<MsgPtr> OnMsg(ServiceHandle sender, SessionId session_id, MsgPtr msg) override {
        if (!msg.IsProtoMsg()) {
            co_return nullptr;
        }
        auto func = AgentLogicHandler::Instance().GetLogicHandle(msg.GetTypeKey());
        if (!func) {
            logger().Err("Agent logic handle not found.");
            co_return nullptr;
        }
        co_return co_await(*func)(this, std::move(msg));
    }

public:
    void SendToClient(ProtoMsgUnique proto_msg) {
        Reply(gateway_, agent_id_, std::move(proto_msg));
    }

    SessionId agent_id() const { return agent_id_; }

private:
    ServiceHandle gateway_;
    SessionId agent_id_;
};

#define MILLION_AGENT_LOGIC_REGISTER_PROTO(PROTO_FILE_NAME, MSG_EXT_ID_, SUB_MSG_EXT_ID_) \
    const bool _MILLION_AGENT_LOGIC_HANDLE_REGISTER_PROTO_##MSG_ID_ = \
        [] { \
            ::million::agent::AgentLogicHandler::Instance().RegisterProto(PROTO_FILE_NAME, MSG_EXT_ID_, SUB_MSG_EXT_ID_); \
            return true; \
        }() \

#define MILLION_AGENT_LOGIC_HANDLE(AGENT_SERVICE_TYPE_, AGENT_SERVICE_NAME_, MSG_TYPE_, MSG_PTR_NAME_) \
    ::million::Task<::million::MsgPtr> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(AGENT_SERVICE_TYPE_* AGENT_SERVICE_NAME_, ::million::MsgPtr msg_, const MSG_TYPE_* msg); \
    ::million::Task<::million::MsgPtr> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_I(::million::agent::AgentService* agent, ::million::MsgPtr msg_) { \
        auto msg = msg_.GetMsg<MSG_TYPE_>(); \
        return _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(static_cast<AGENT_SERVICE_TYPE_*>(agent), std::move(msg_), msg); \
    } \
    const bool MILLION_AGENT_LOGIC_HANDLE_REGISTER_##MSG_TYPE_ =  \
        [] { \
            ::million::agent::AgentLogicHandler::Instance().RegisterLogicHandle(::million::GetMsgTypeKey<MSG_TYPE_>(), \
                _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_I); \
            return true; \
        }(); \
    ::million::Task<::million::MsgPtr> _MILLION_AGENT_LOGIC_HANDLE_##MSG_TYPE_##_II(AGENT_SERVICE_TYPE_* agent, ::million::MsgPtr msg_ptr, const MSG_TYPE_* MSG_PTR_NAME_)


} // namespace agent
} // namespace million