#pragma once

#include <yaml-cpp/yaml.h>

#include <million/iservice.h>

#include <gateway/api.h>
#include <gateway/gateway.h>

#include "gateway_server.h"

namespace million {
namespace gateway {

namespace protobuf = google::protobuf; 

MILLION_MSG_DEFINE(, GatewayTcpConnection, (UserSessionShared) user_session)
MILLION_MSG_DEFINE(, GatewayTcpRecvPacket, (UserSessionShared) user_session, (net::Packet) packet)

MILLION_MSG_DEFINE(, GatewayPersistentUserSession, (UserSessionShared) user_session);

// 跨节点：
    // 网关服务分为主服务及代理服务，主服务负责收包，部署在网关节点上
    // 其他需要接收网关转发的包的节点，需要部署一个代理服务，代理服务负责接收集群服务转发来的网关主服务发的包，再转发给节点内的其他服务
class GatewayService : public IService {
    MILLION_SERVICE_DEFINE(GatewayService);

public:
    using Base = IService;
    GatewayService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual bool OnInit() override {
        return imillion().SetServiceName(service_handle(), kGatewayServiceName);
    }


    virtual Task<MsgPtr> OnStart(ServiceHandle sender, SessionId session_id, MsgPtr with_msg) override {
        const auto& settings = imillion().YamlSettings();
        const auto& gateway_settings = settings["gateway"];
        if (!gateway_settings) {
            TaskAbort("cannot find 'gateway'.");
            co_return nullptr;
        }
        const auto& port_settings = gateway_settings["port"];
        if (!port_settings) {
            TaskAbort("cannot find 'gateway.port'.");
            co_return nullptr;
        }
        const auto port = port_settings.as<uint16_t>();

        // io线程回调，发给work线程处理
        server_.set_on_connection([this](auto&& connection) -> asio::awaitable<void> {
            Send<GatewayTcpConnection>(service_handle(), std::move(std::static_pointer_cast<UserSession>(connection)));
            co_return;
            });
        server_.set_on_msg([this](auto&& connection, auto&& packet) -> asio::awaitable<void> {
            Send<GatewayTcpRecvPacket>(service_handle(), std::move(std::static_pointer_cast<UserSession>(connection)), std::move(packet));
            co_return;
            });
        server_.Start(port);
        logger().Info("Gateway start.");
        co_return nullptr;
    }

    MILLION_MSG_HANDLE(GatewayPersistentUserSession, msg) {
        auto& user_session = *msg->user_session;
        do {
            auto recv_msg = co_await ::million::SessionAwaiterBase(session_id, ::million::kSessionNeverTimeout, false);
            // imillion().SendTo(sender, service_handle(), session_id, std::move(recv_msg));
            if (recv_msg.IsProtoMsg()) {
                logger().Trace("Gateway Recv ProtoMessage: {}.", session_id);
                auto header_packet = net::Packet(kGatewayHeaderSize);

                auto proto_msg = std::move(recv_msg.GetProtoMsg());
                auto packet = imillion().proto_mgr().codec().EncodeMessage(*proto_msg);
                if (!packet) {
                    logger().Err("Gateway Recv ProtoMessage EncodeMessage failed: {}.", session_id);
                    continue;
                }
                auto span = net::PacketSpan(header_packet);
                user_session.Send(std::move(header_packet), span, header_packet.size() + packet->size());
                span = net::PacketSpan(*packet);
                user_session.Send(std::move(*packet), span, 0);
            }
            else if (recv_msg.IsType<GatewaySendPacket>()) {
                logger().Trace("GatewaySendPacket: {}.", session_id);
                auto header_packet = net::Packet(kGatewayHeaderSize);
                auto msg = recv_msg.GetMutMsg<GatewaySendPacket>();
                auto span = net::PacketSpan(header_packet);
                user_session.Send(std::move(header_packet), span, header_packet.size() + msg->packet.size());
                span = net::PacketSpan(msg->packet);
                user_session.Send(std::move(msg->packet), span, 0);
            }
            else if (recv_msg.IsType<GatewaySureAgent>()) {
                auto msg = recv_msg.GetMutMsg<GatewaySureAgent>();
                user_session.set_agent_handle(std::move(msg->agent_service));
            }
            else if (recv_msg.IsType<GatewayPersistentUserSession>()) {
                break;
            }
        } while (true);
        co_return nullptr;
    }

    MILLION_MSG_HANDLE(GatewayTcpConnection, msg) {
        auto& user_session = *msg->user_session;

        auto& ep = user_session.remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());

        if (user_session.Connected()) {
            // 开启持久会话，这里设计上，Send 创建的 session_id，则视作 agent_id
            auto agent_id = Send<GatewayPersistentUserSession>(service_handle(), std::move(msg->user_session));
            user_session.set_agent_id(agent_id.value());

            logger().Debug("Gateway connection establishment, agent_id:{}, ip: {}, port: {}", user_session.agent_id(), ip, port);
        }
        else {
            // 停止持久会话
            Reply<GatewayPersistentUserSession>(service_handle(), user_session.agent_id(), std::move(msg->user_session));

            logger().Debug("Gateway Disconnection: ip: {}, port: {}", ip, port);
        }
        co_return nullptr;
    }

    MILLION_MSG_HANDLE(GatewayTcpRecvPacket, msg) {
        auto& user_session = *msg->user_session;
        auto agent_id = user_session.agent_id();

        logger().Trace("GatewayTcpRecvPacketMsg: {}, {}.", agent_id, msg->packet.size());

        if (user_session.token() == kInvaildToken) {
            // 连接没token，但是发来了token，当成断线重连处理
            // user_session.set_token(info.token);

            // todo: 需要断开原先token指向的连接
        }
        // 没有token

        if (msg->packet.size() < kGatewayHeaderSize){
            logger().Warn("GatewayTcpRecvPacketMsg size err: {}, {}.", agent_id, msg->packet.size());
            co_return nullptr;
        }

        auto span = net::PacketSpan(msg->packet.begin() + kGatewayHeaderSize, msg->packet.end());

        // 将消息转发给本机节点的其他服务
        auto res = imillion().proto_mgr().codec().DecodeMessage(span);
        if (!res) {
            logger().Warn("GatewayTcpRecvPacketMsg unresolvable message: {}, {}.", agent_id, msg->packet.size());
            co_return nullptr;
        }

        // if (session->token == kInvaildToken)

        // 指定user_session_id，目标在通过Reply回包时，会在GatewayPersistentUserSessionMsg中循环接收处理
        if (!user_session.agent_handle().lock()) {
            logger().Trace("packet send to user service.");
            SendTo(user_service_, agent_id, std::move(res->msg));
        }
        else {
            logger().Trace("packet send to agent service.");
            SendTo(user_session.agent_handle(), agent_id, std::move(res->msg));
        }
        co_return nullptr;
    }

    MILLION_MSG_HANDLE(GatewayRegisterUserServiceReq, msg) {
        logger().Trace("GatewayRegisterUserServiceReq.");
        user_service_ = msg->user_service;
        co_return make_msg<GatewayRegisterUserServiceResp>();
    }


private:
    GatewayServer server_;
    TokenGenerator token_generator_;
    ServiceHandle user_service_;

    static constexpr uint32_t kGatewayHeaderSize = 8;
};

} // namespace gateway
} // namespace million
