#pragma once

#include <yaml-cpp/yaml.h>

#include <million/iservice.h>

#include <gateway/api.h>
#include <gateway/gateway_msg.h>

#include "gateway_server.h"

#define MILLION_CS_PROTO_MSG_DISPATCH() MILLION_PROTO_MSG_DISPATCH(Cs, ::million::gateway::UserSessionHandle)
#define MILLION_CS_PROTO_MSG_ID(MSG_ID_) MILLION_PROTO_MSG_ID(Cs, MSG_ID_)
#define MILLION_CS_PROTO_MSG_HANDLE(NAMESPACE_, SUB_MSG_ID_, MSG_TYPE_, MSG_PTR_NAME_) MILLION_PROTO_MSG_HANDLE(Cs::##NAMESPACE_, ::million::gateway::UserSessionHandle, SUB_MSG_ID_, MSG_TYPE_, MSG_PTR_NAME_)

namespace million {
namespace gateway {

namespace protobuf = google::protobuf; 

class AgentService : public IService {
public:
    using Base = IService;
    AgentService(IMillion* imillion, uint64_t user_context_id)
        : Base(imillion)
        , user_context_id_(user_context_id) {}

private:
    MILLION_MSG_DISPATCH(AgentService);

    MILLION_MSG_HANDLE(GatewayRecvPacketMsg, msg) {
        auto res = g_agent_proto_codec->DecodeMessage(msg->packet);
        if (!res) {
            LOG_ERR("DecodeMessage failed, msg_id:{}, sub_msg_id:{}", res->msg_id, res->sub_msg_id);
            co_return;
        }
        auto iter = g_agent_logic_handle_map->find(ProtoCodec::CalcKey(res->msg_id, res->sub_msg_id));
        if (iter == g_agent_logic_handle_map->end()) {
            LOG_ERR("Agent logic handle not found, msg_id:{}, sub_msg_id:{}", res->msg_id, res->sub_msg_id);
            co_return;
        }
        co_await iter->second(this, *res->proto_msg);
        co_return;
    }

public:
    void SendProtoMsg(const protobuf::Message& proto_msg) {
        auto res = g_agent_proto_codec->EncodeMessage(proto_msg);
        if (!res) {
            LOG_ERR("EncodeMessage failed: type:{}.", typeid(proto_msg).name());
            return;
        }
        Send<GatewaySendPacketMsg>(gateway_, user_context_id_, *res);
    }

private:

    ServiceHandle gateway_;
    UserContextId user_context_id_;
};


class NodeMgrService : public IService {
public:
    using Base = IService;
    using Base::Base;

    MILLION_MSG_DISPATCH(NodeMgrService);

    MILLION_MSG_HANDLE(NodeMgrNewAgentMsg, msg) {
        auto handle = imillion_->NewService<AgentService>(msg->context_id);
        if (!handle) {
            LOG_ERR("NewService AgentService failed.");
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
        auto handle = imillion_->GetServiceByName("GatewayService");
        if (!handle) {
            LOG_ERR("GatewayService not found.");
            return false;
        }
        gateway_ = *handle;

        handle = imillion_->GetServiceByName("NodeMgrService");
        if (!handle) {
            LOG_ERR("NodeMgrService not found.");
            return false;
        }
        node_mgr_ = *handle;
    }

    MILLION_MSG_DISPATCH(AgentMgrService);

    MILLION_MSG_HANDLE(AgentMgrLoginMsg, msg) {
        auto agent_msg = co_await Call<NodeMgrNewAgentMsg>(node_mgr_, msg->context_id, std::nullopt);
        msg->agent_handle = std::move(agent_msg->agent_handle);

        Send<GatewaySureAgentMsg>(gateway_, msg->context_id, *msg->agent_handle);

        Reply(std::move(msg));
        co_return;
    }

private:
    ServiceHandle gateway_;
    ServiceHandle node_mgr_;
};


MILLION_MSG_DEFINE(, GatewayTcpConnectionMsg, (net::TcpConnectionShared) connection)
MILLION_MSG_DEFINE(, GatewayTcpRecvPacketMsg, (net::TcpConnectionShared) connection, (net::Packet) packet)

// 网关服务有两种模式
// 集群、非集群
// 集群模式：
    // 网关服务分为主服务及代理服务，主服务负责收包，部署在单节点上
    // 其他需要接收网关转发的包的节点，需要部署一个代理服务，代理服务负责接收集群服务转发来的网关主服务发的包，再转发给节点内的其他服务
class GatewayService : public IService {
public:
    using Base = IService;
    GatewayService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual bool OnInit() override {
        LOG_INFO("Gateway init start.");
        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto&& connection) -> asio::awaitable<void> {
            Send<GatewayTcpConnectionMsg>(service_handle(), connection);
            co_return;
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) -> asio::awaitable<void> {
            Send<GatewayTcpRecvPacketMsg>(service_handle(), connection, std::move(packet));
            co_return;
        });
        const auto& config = imillion_->YamlConfig();
        const auto& gateway_config = config["gateway"];
        if (!gateway_config) {
            LOG_ERR("cannot find 'gateway'.");
            return false;
        }
        const auto& port_config = gateway_config["port"];
        if (!port_config)
        {
            LOG_ERR("cannot find 'gateway.port'.");
            return false;
        }
        server_.Start(port_config.as<uint16_t>());
        LOG_INFO("Gateway init success.");
        return true;
    }

    MILLION_MSG_DISPATCH(GatewayService);

    MILLION_MSG_HANDLE(GatewayTcpConnectionMsg, msg) {
        auto& connection = msg->connection;
        auto session = static_cast<UserSession*>(msg->connection.get());

        auto& ep = connection->remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());

        if (connection->Connected()) {
            auto user_context_id = ++user_context_id_;
            session->info().user_context_id = user_context_id;
            users_.emplace(session->info().user_context_id, session);
            LOG_DEBUG("Gateway connection establishment: ip: {}, port: {}", ip, port);
        }
        else {
            users_.erase(session->info().user_context_id);
            LOG_DEBUG("Gateway Disconnection: ip: {}, port: {}", ip, port);
        }
        co_return;
    }

    MILLION_MSG_HANDLE(GatewayTcpRecvPacketMsg, msg) {
        auto session = static_cast<UserSession*>(msg->connection.get());
        auto session_handle = UserSessionHandle(session);

        if (session->info().token == kInvaildToken) {
            // 连接没token，但是发来了token，当成断线重连处理
            // session->info().token = info.token;

            // todo: 需要断开原先token指向的连接
        }
        auto user_context_id = session->info().user_context_id;
        // 没有token
        auto span = net::PacketSpan(msg->packet.begin() + kGatewayHeaderSize, msg->packet.end());
        auto iter = agent_services_.find(session->info().user_context_id);

        // if (session->info().token == kInvaildToken) {
        if (iter == agent_services_.end()) {
            Send<GatewayRecvPacketMsg>(user_service_, user_context_id, std::move(msg->packet), span);
        }
        else {
            Send<GatewayRecvPacketMsg>(iter->second, user_context_id, std::move(msg->packet), span);
        }
        co_return;
    }

    MILLION_MSG_HANDLE(GatewayRegisterUserServiceMsg, msg) {
        user_service_ = msg->user_service;
        co_return;
    }

    MILLION_MSG_HANDLE(GatewaySureAgentMsg, msg) {
        agent_services_.emplace(msg->context_id, msg->agent_service);
        co_return;
    }

    MILLION_MSG_HANDLE(GatewaySendPacketMsg, msg) {
        auto iter = users_.find(msg->context_id);
        if (iter == users_.end()) {
            co_return;
        }

        auto header_packet = net::Packet(kGatewayHeaderSize);
        iter->second->Send(std::move(header_packet), net::PacketSpan(header_packet.begin(), header_packet.end()), header_packet.size() + msg->packet.size());
        iter->second->Send(std::move(msg->packet), net::PacketSpan(msg->packet.begin(), msg->packet.end()), 0);
        co_return;
    }

private:
    GatewayServer server_;
    TokenGenerator token_generator_;
    ServiceHandle user_service_;
    // 需要改掉UserSession*
    std::atomic<UserContextId> user_context_id_ = 0;
    std::unordered_map<UserContextId, UserSession*> users_;
    std::unordered_map<UserContextId, ServiceHandle> agent_services_;

    static constexpr uint32_t kGatewayHeaderSize = 8;
};

} // namespace gateway
} // namespace million
