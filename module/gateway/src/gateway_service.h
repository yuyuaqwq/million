#pragma once

#include <yaml-cpp/yaml.h>

#include <million/iservice.h>

#include <gateway/api.h>
#include <gateway/gateway.h>

#include "gateway_server.h"

namespace million {
namespace gateway {

namespace protobuf = google::protobuf; 

MILLION_MSG_DEFINE(, GatewayTcpConnectionMsg, (UserSessionShared) user_session)
MILLION_MSG_DEFINE(, GatewayTcpRecvPacketMsg, (UserSessionShared) user_session, (net::Packet) packet)

MILLION_MSG_DEFINE(, ClusterPersistentUserSessionMsg, (UserSessionShared) user_session);

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

    virtual bool OnInit(::million::MsgUnique msg) override {
        logger().Info("Gateway init start.");
        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto&& connection) -> asio::awaitable<void> {
            Send<GatewayTcpConnectionMsg>(service_handle(), std::move(std::static_pointer_cast<UserSession>(connection)));
            co_return;
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) -> asio::awaitable<void> {
            Send<GatewayTcpRecvPacketMsg>(service_handle(), std::move(std::static_pointer_cast<UserSession>(connection)), std::move(packet));
            co_return;
        });
        const auto& config = imillion().YamlConfig();
        const auto& gateway_config = config["gateway"];
        if (!gateway_config) {
            logger().Err("cannot find 'gateway'.");
            return false;
        }
        const auto& port_config = gateway_config["port"];
        if (!port_config)
        {
            logger().Err("cannot find 'gateway.port'.");
            return false;
        }
        server_.Start(port_config.as<uint16_t>());
        logger().Info("Gateway init success.");
        return true;
    }

    MILLION_MSG_DISPATCH(GatewayService);

    MILLION_MSG_HANDLE(ClusterPersistentUserSessionMsg, msg) {
        auto& user_session = *msg->user_session;
        do {
            auto recv_msg = co_await ::million::SessionAwaiterBase(session_id, ::million::kSessionNeverTimeout, false);
            auto msg_type_key = recv_msg.GetTypeKey();
            // imillion().SendTo(sender, service_handle(), session_id, std::move(recv_msg));
            if (recv_msg.IsType<GatewaySendPacketMsg>()) {
                logger().Trace("GatewaySendPacketMsg: {}.", session_id);
                auto header_packet = net::Packet(kGatewayHeaderSize);
                auto msg = recv_msg.get<GatewaySendPacketMsg>();
                user_session.Send(std::move(header_packet), net::PacketSpan(header_packet), header_packet.size() + msg->packet.size());
                user_session.Send(std::move(msg->packet), net::PacketSpan(msg->packet), 0);
            }
            else if (recv_msg.IsType<GatewaySureAgentMsg>()) {
                auto msg = recv_msg.get<GatewaySureAgentMsg>();
                user_session.set_agent(std::move(msg->agent_service));
            }
            else if (recv_msg.IsType<ClusterPersistentUserSessionMsg>()) {
                break;
            }
        } while (true);
        co_return;
    }

    MILLION_MSG_HANDLE(GatewayTcpConnectionMsg, msg) {
        auto& user_session = *msg->user_session;

        auto& ep = user_session.remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());

        if (user_session.Connected()) {
            // 开启持久会话
            auto user_session_id = Send<ClusterPersistentUserSessionMsg>(service_handle(), std::move(msg->user_session));
            user_session.set_user_session_id(user_session_id);

            logger().Debug("Gateway connection establishment, user_session_id:{}, ip: {}, port: {}", user_session_id, ip, port);
        }
        else {
            // 停止持久会话
            Reply<ClusterPersistentUserSessionMsg>(service_handle(), user_session.user_session_id(), std::move(msg->user_session));

            logger().Debug("Gateway Disconnection: ip: {}, port: {}", ip, port);
        }
        co_return;
    }

    MILLION_MSG_HANDLE(GatewayTcpRecvPacketMsg, msg) {
        auto& user_session = *msg->user_session;

        auto user_session_id = user_session.user_session_id();

        logger().Trace("GatewayTcpRecvPacketMsg: {}, {}.", user_session_id, msg->packet.size());

        if (user_session.token() == kInvaildToken) {
            // 连接没token，但是发来了token，当成断线重连处理
            // user_session.set_token(info.token);

            // todo: 需要断开原先token指向的连接
        }
        // 没有token
        auto span = net::PacketSpan(msg->packet.begin() + kGatewayHeaderSize, msg->packet.end());

        // if (session->token == kInvaildToken) {
        if (!user_session.agent().lock()) {
            logger().Trace("packet send to user service.");
            SendTo<GatewayRecvPacketMsg>(user_service_, user_session_id, std::move(msg->packet), span);
        }
        else {
            logger().Trace("packet send to agent service.");
            Send<GatewayRecvPacketMsg>(user_session.agent(), std::move(msg->packet), span);
        }
        co_return;
    }

    MILLION_MSG_HANDLE(GatewayRegisterUserServiceMsg, msg) {
        logger().Trace("GatewayRegisterUserServiceMsg.");
        user_service_ = msg->user_service;
        // Reply(sender, session_id, std::move(msg));
        co_return;
    }

private:
    GatewayServer server_;
    TokenGenerator token_generator_;
    ServiceHandle user_service_;

    static constexpr uint32_t kGatewayHeaderSize = 8;
};

} // namespace gateway
} // namespace million
