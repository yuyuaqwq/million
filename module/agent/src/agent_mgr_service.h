#pragma once

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <agent/api.h>
#include <agent/agent.h>

namespace million {
namespace agent {

class AgentMgrService : public IService {
public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit(::million::MsgPtr msg) override {
        auto handle = imillion().GetServiceByName("NodeMgrService");
        if (!handle) {
            logger().Err("NodeMgrService not found.");
            return false;
        }
        node_mgr_ = *handle;

        handle = imillion().GetServiceByName("GatewayService");
        if (!handle) {
            logger().Err("GatewayService not found.");
            return false;
        }
        gateway_ = *handle;

        imillion().SetServiceName(service_handle(), "AgentMgrService");
    }

    MILLION_MSG_DISPATCH(AgentMgrService);

    MILLION_MSG_HANDLE(AgentMgrLoginMsg, msg) {
        auto mut_msg = msg_ptr.GetMutableMsg<AgentMgrLoginMsg>();

        auto agent_msg = co_await Call<NewAgentMsg>(node_mgr_, msg->user_session_id, std::nullopt);
        mut_msg->agent_handle = std::move(agent_msg->agent_handle);
        Reply<gateway::GatewaySureAgentMsg>(gateway_, msg->user_session_id, *msg->agent_handle);
        co_return std::move(msg_ptr);
    }

private:
    ServiceHandle gateway_;
    ServiceHandle node_mgr_;
};

} // namespace agent
} // namespace million
