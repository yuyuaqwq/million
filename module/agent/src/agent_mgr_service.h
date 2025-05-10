#pragma once

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>

#include <gateway/gateway.h>

#include <agent/api.h>
#include <agent/agent.h>

namespace million {
namespace agent {

class AgentMgrService : public IService {
    MILLION_SERVICE_DEFINE(AgentMgrService);

public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override {
        imillion().SetServiceName(service_handle(), kAgentMgrServiceName);
        return true;
    }

    virtual Task<MessagePointer> OnStart(ServiceHandle sender, SessionId session_id, MessagePointer with_msg) override {
        auto handle = imillion().GetServiceByName("NodeMgrService");
        TaskAssert(handle, "NodeMgrService not found.");
        
        node_mgr_ = *handle;

        handle = imillion().GetServiceByName(gateway::kGatewayServiceName);
        TaskAssert(handle, "GatewayService not found.");
        
        gateway_ = *handle;
        co_return nullptr;
    }

    MILLION_MESSAGE_HANDLE(AgentMgrLoginReq, msg) {
        auto new_resp = co_await Call<NewAgentReq, NewAgentResp>(node_mgr_, msg->agent_id);
        if (!new_resp->agent_handle) {
            TaskAbort("New agent failed.");
        }
        Reply<gateway::GatewaySureAgent>(gateway_, msg->agent_id, *new_resp->agent_handle);
        co_return make_message<AgentMgrLoginResp>(std::move(new_resp->agent_handle));
    }

private:
    ServiceHandle gateway_;
    ServiceHandle node_mgr_;
};

} // namespace agent
} // namespace million
