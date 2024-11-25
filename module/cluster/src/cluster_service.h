#pragma once

#include <million/iservice.h>

#include <protogen/ss/ss_cluster.pb.h>

#include <cluster/cluster_msg.h>

#include "cluster_server.h"

namespace million {
namespace cluster {

namespace Ss = ::Million::Proto::Ss;

MILLION_MSG_DEFINE(, ClusterTcpConnectionMsg, (net::TcpConnectionShared) connection)
MILLION_MSG_DEFINE(, ClusterTcpRecvPacketMsg, (net::TcpConnectionShared) connection, (net::Packet) packet)
MILLION_MSG_DEFINE(, ClusterTcpConnectToNotifyMsg, (net::TcpConnectionShared) connection)

class ClusterService : public IService {
public:
    using Base = IService;
    ClusterService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual bool OnInit() override {
        // io�̻߳ص�������work�̴߳���
        server_.set_on_connection([this](const net::TcpConnectionShared& connection) -> asio::awaitable<void> {
            Send<ClusterTcpConnectionMsg>(service_handle(), connection);
            co_return;
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) -> asio::awaitable<void> {
            Send<ClusterTcpRecvPacketMsg>(service_handle(), connection, std::move(packet));
            co_return;
        });

        const auto& config = imillion_->YamlConfig();
        const auto& cluster_config = config["cluster"];
        if (!cluster_config) {
            LOG_ERR("cannot find 'cluster'.");
            return false;
        }

        const auto& name_config = cluster_config["name"];
        if (!name_config)
        {
            LOG_ERR("cannot find 'cluster.name'.");
            return false;
        }
        node_name_ = name_config.as<std::string>();

        const auto& port_config = cluster_config["port"];
        if (!port_config)
        {
            LOG_ERR("cannot find 'cluster.port'.");
            return false;
        }
        server_.Start(port_config.as<uint16_t>());
        return true;
    }

    MILLION_MSG_DISPATCH(ClusterService);

    MILLION_MSG_HANDLE(ClusterTcpConnectionMsg, msg) {
        auto& ep = msg->connection->remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());
        if (msg->connection->Connected()) {
            LOG_DEBUG("Connection establishment: ip: {}, port: {}", ip, port);
        }
        else {
            LOG_DEBUG("Disconnection: ip: {}, port: {}", ip, port);
        }

        // ��������������Ҳ�����Ǳ����յ����ӵģ�����ȥ�ز������ﴦ��
        // ����Ͷ�ݵ���ʱ���񣬱���10s��û�����ֳɹ��ͶϿ�����
        
        co_return;
    }

    MILLION_MSG_HANDLE(ClusterTcpRecvPacketMsg, msg) {
        auto node_session = msg->connection->get_ptr<NodeSession>();
        if (node_session->info().node_name.empty()) {
            // δ���������֤�����ӣ����������֤����Ŀ��ڵ㷢�������
            Ss::ClusterHandshake handshake;
            if (!handshake.ParseFromArray(msg->packet.data(), msg->packet.size())) {
                auto& ep = msg->connection->remote_endpoint();
                auto ip = ep.address().to_string();
                auto port = std::to_string(ep.port());
                LOG_WARN("Invalid ClusterHandshake: ip: {}, port: {}", ip, port);
                msg->connection->Close();
                co_return;
            }
            CreateNode(handshake.src_node(), std::move(msg->connection), false);
            co_return;
        }

        // ����ͷ��
        Ss::ClusterHeader header;
        header.ParseFromArray(msg->packet.data(), msg->packet.size());

        auto& src_service = header.src_service();
        auto& target_service = header.target_service();

        auto target_service_handle = imillion_->GetServiceByUniqueNum(target_service);
        if (target_service_handle) {
            // ����Ҫ��ȡ��Դ�ڵ�
            Send<ClusterRecvPacketMsg>(*target_service_handle, std::string(), std::move(src_service), std::move(msg->packet), net::PacketSpan(msg->packet.begin() + header.ByteSize(), msg->packet.end()));
        }
        co_return;
    }

    MILLION_MSG_HANDLE(ClusterSendPacketMsg, msg) {
        EndPointRes end_point;
        auto connection_ptr = FindNode(msg->target_node, &end_point);
        if (!connection_ptr && end_point.ip.empty()) {
            LOG_ERR("Node cannot be found: {}.", msg->target_node);
            co_return;
        }
        if (!connection_ptr) {
            auto session_id = AllocSessionId();
            auto& io_context = imillion_->NextIoContext();
            // ����Ҫ�������ӹ����������������ýڵ����Ϣ�Ĵ���
            asio::co_spawn(io_context.get_executor(), [this, session_id, end_point = std::move(end_point)]() mutable -> asio::awaitable<void> {
                auto connection_opt = co_await server_.ConnectTo(end_point.ip, end_point.port);
                if (!connection_opt) {
                    co_return;
                }
                // ���﷢��֪ͨ����֪���ӳɹ�
                Reply<ClusterTcpConnectToNotifyMsg>(service_handle(), session_id, *connection_opt);
            }, asio::detached);

            // ����ȴ�֪ͨ��Ȼ���������
            auto notify_msg = co_await Recv<ClusterTcpConnectToNotifyMsg>(session_id);

            auto& connection = CreateNode(msg->target_node, std::move(notify_msg->connection), true);

            // ���������֤
            Ss::ClusterHandshake handshake;
            handshake.set_src_node(node_name_);
            auto packet = net::Packet(handshake.ByteSize());
            handshake.SerializeToArray(packet.data(), packet.size());
            connection->Send(std::move(packet));

            connection_ptr = connection.get();
        }
        // ׷�Ӽ�Ⱥͷ��
        Ss::ClusterHeader header;
        header.set_src_service(std::move(msg->src_service));
        header.set_target_service(std::move(msg->target_service));
        auto header_packet = net::Packet(header.ByteSize());
        header.SerializeToArray(header_packet.data(), header_packet.size());
        connection_ptr->Send(std::move(header_packet), net::PacketSpan(header_packet.begin(), header_packet.end()), header_packet.size() + msg->packet.size());
        connection_ptr->Send(std::move(msg->packet), net::PacketSpan(msg->packet.begin(), msg->packet.end()), 0);
        co_return;
    }

private:
    net::TcpConnectionShared& CreateNode(const NodeUniqueName& target_node_name, net::TcpConnectionShared&& connection, bool active) {
        // �����������Ҫ����ʧ��
        auto res = nodes_.emplace(target_node_name, connection);
        if (res.second) {
            auto node_session = connection->get_ptr<NodeSession>();
            node_session->info().node_name = target_node_name;
            LOG_INFO("CreateNode: {}", target_node_name);
        }
        else {
            // �Ѵ��ڣ��Ƚ����߽ڵ������ֵ���ѡ��Ͽ�ĳһ������
            bool disconnect_old;
            if (active) {
                // ��������
                disconnect_old = node_name_ > target_node_name;
            }
            else {
                // ��������
                disconnect_old = target_node_name > node_name_;
            }

            if (disconnect_old) {
                // �Ͽ������ӣ�����������
                res.first->second->Close();
                res.first->second = connection;
                LOG_INFO("Duplicate connection, disconnect old connection: {}", target_node_name);
            }
            else {
                // �Ͽ�������
                connection->Close();
                LOG_INFO("Duplicate connection, disconnect new connection: {}", target_node_name);
            }
        }
        return res.first->second;
    }

    struct EndPointRes {
        std::string ip;
        std::string port;
    };
    net::TcpConnection* FindNode(NodeUniqueName node_name, EndPointRes* end_point) {
        auto iter = nodes_.find(node_name);
        if (iter != nodes_.end()) {
            return iter->second.get();
        }

        // ���Դ������ļ��в��Ҵ˽ڵ�
        const auto& config = imillion_->YamlConfig();
        const auto& cluster_config = config["cluster"];
        if (!cluster_config) {
            LOG_ERR("cannot find 'cluster'.");
            return nullptr;
        }

        const auto& nodes = cluster_config["nodes"];
        if (!nodes) {
            LOG_ERR("cannot find 'cluster.nodes'.");
            return nullptr;
        }

        for (auto node : nodes) {
            auto node_name = node.first.as<std::string>();
            if (node_name != node_name) continue;

            end_point->ip = node.second["ip"].as<std::string>();
            end_point->port = node.second["port"].as<std::string>();

            break;
        }
        return nullptr;
    }

private:
    ClusterServer server_;
    NodeUniqueName node_name_;
    std::unordered_map<NodeUniqueName, net::TcpConnectionShared> nodes_;
};

} // namespace gateway
} // namespace million
