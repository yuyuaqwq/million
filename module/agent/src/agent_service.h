#pragma once

#include <yaml-cpp/yaml.h>

#include <million/iservice.h>

#include <agent/api.h>
#include <agent/agent.h>

namespace million {
namespace agent {

MILLION_MSG_DEFINE(, NodeMgrNewAgentMsg, (uint64_t) context_id, (std::optional<ServiceHandle>) agent_handle);

class NodeMgrService : public IService {
public:
    using Base = IService;
    using Base::Base;

    MILLION_MSG_DISPATCH(NodeMgrService);

    MILLION_CPP_MSG_HANDLE(NodeMgrNewAgentMsg, msg) {
        auto handle = imillion().NewService<AgentService>(msg->context_id);
        if (!handle) {
            logger().Err("NewService AgentService failed.");
            co_return;
        }
        msg->agent_handle = std::move(*handle);
        Reply(std::move(msg));
        co_return;
    }

private:
};

class AgentMgrService : public IService {
public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override {
        auto handle = imillion().GetServiceByName("GatewayService");
        if (!handle) {
            logger().Err("GatewayService not found.");
            return false;
        }
        gateway_ = *handle;

        handle = imillion().GetServiceByName("NodeMgrService");
        if (!handle) {
            logger().Err("NodeMgrService not found.");
            return false;
        }
        node_mgr_ = *handle;
    }

    MILLION_MSG_DISPATCH(AgentMgrService);

    MILLION_CPP_MSG_HANDLE(AgentMgrLoginMsg, msg) {
        auto agent_msg = co_await Call<NodeMgrNewAgentMsg>(node_mgr_, msg->context_id, std::nullopt);
        msg->agent_handle = std::move(agent_msg->agent_handle);

        Send<gateway::GatewaySureAgentMsg>(gateway_, msg->context_id, *msg->agent_handle);

        Reply(std::move(msg));
        co_return;
    }

private:
    ServiceHandle gateway_;
    ServiceHandle node_mgr_;
};

} // namespace agent
} // namespace million
