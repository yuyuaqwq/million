#pragma once

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <gateway/gateway.h>

#include <agent/api.h>
#include <agent/agent.h>

namespace million {
namespace agent {

class AgentMgrService : public IService {
public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override {
        imillion().SetServiceName(service_handle(), kAgentMgrServiceName);
        return true;
    }

    virtual Task<MsgPtr> OnStart(ServiceHandle sender, SessionId session_id, MsgPtr with_msg) override {
        auto handle = imillion().GetServiceByName("NodeMgrService");
        TaskAssert(handle, "NodeMgrService not found.");
        
        node_mgr_ = *handle;

        handle = imillion().GetServiceByName(gateway::kGatewayServiceName);
        TaskAssert(handle, "GatewayService not found.");
        
        gateway_ = *handle;
        co_return nullptr;
    }

    MILLION_MSG_DISPATCH(AgentMgrService);

    MILLION_MUT_MSG_HANDLE(AgentMgrLoginMsg, msg) {
        auto agent_msg = co_await Call<NewAgentMsg>(node_mgr_, msg->agent_id, std::nullopt);
        if (!agent_msg->agent_handle) {
            TaskAbort("New agent failed.");
        }
        msg->agent_handle = std::move(agent_msg->agent_handle);
        Reply<gateway::GatewaySureAgentMsg>(gateway_, msg->agent_id, *msg->agent_handle);
        co_return std::move(msg_);
    }

private:
    ServiceHandle gateway_;
    ServiceHandle node_mgr_;
};

} // namespace agent
} // namespace million
