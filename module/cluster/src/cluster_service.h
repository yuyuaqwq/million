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
            auto node_session = msg->connection->get_ptr<NodeSession>();
            LOG_DEBUG("Disconnection: ip: {}, port: {}, cur_node: {}, target_node : {}", ip, port, node_name_, node_session->info().node_name);
            DeleteNodeSession(node_session);
        }

        // ��������������Ҳ�����Ǳ����յ����ӵģ�����ȥ�ز������ﴦ��
        // ����Ͷ�ݵ���ʱ���񣬱���10s��û�����ֳɹ��ͶϿ�����
        
        co_return;
    }

    MILLION_MSG_HANDLE(ClusterTcpRecvPacketMsg, msg) {
        auto& ep = msg->connection->remote_endpoint();
        auto ip = ep.address().to_string();
        auto port = std::to_string(ep.port());

        uint32_t header_size = 0;
        if (msg->packet.size() < sizeof(header_size)) {
            LOG_WARN("Invalid header size: ip: {}, port: {}", ip, port);
            msg->connection->Close();
            co_return;
        }
        header_size = *reinterpret_cast<uint32_t*>(msg->packet.data());
        header_size = asio::detail::socket_ops::network_to_host_long(header_size);

        Ss::Cluster::ClusterMsg cluster_msg;
        if (!cluster_msg.ParseFromArray(msg->packet.data() + sizeof(header_size), header_size)) {
            LOG_WARN("Invalid ClusterMsg: ip: {}, port: {}", ip, port);
            msg->connection->Close();
            co_return;
        }

        auto node_session = msg->connection->get_ptr<NodeSession>();

        switch (cluster_msg.body_case()) {
        case Ss::Cluster::ClusterMsg::BodyCase::kHandshakeReq: {
            // �յ��������󣬵�ǰ�Ǳ������ӷ�
            auto& req = cluster_msg.handshake_req();
            node_session->info().node_name = req.src_node();

            // ����Ҫ����Ƿ���Ŀ��ڵ�������ӣ�����Ѵ��ڣ�˵���������Ѿ�ʧЧ����Ҫ�Ͽ�
            auto old_node_session = FindNodeSession(req.src_node(), nullptr);
            if (old_node_session) {
                old_node_session->Close();
                LOG_INFO("old connection, ip: {}, port: {}, cur_node: {}, req_src_node: {}", ip, port, node_name_, req.src_node());
            }

            // �ܷ�ƥ�䶼�ذ�
            Ss::Cluster::ClusterMsg cluster_msg;
            auto* res = cluster_msg.mutable_handshake_res();
            res->set_target_node(node_name_);
            auto packet = ProtoMsgToPacket(cluster_msg);

            SendInit(node_session, packet, net::Packet());

            auto span = net::PacketSpan(packet.begin(), packet.end());
            node_session->Send(std::move(packet), span, 0);

            if (node_name_ != req.target_node()) {
                LOG_ERR("Src node name mismatch, ip: {}, port: {}, cur_node: {}, req_target_node: {}", ip, port, node_name_, req.target_node());
                node_session->Close();
                co_return;
            }

            // �����ڵ�
            CreateNodeSession(std::move(msg->connection), false);
            break;
        }
        case Ss::Cluster::ClusterMsg::BodyCase::kHandshakeRes: {
            // �յ�������Ӧ����ǰ���������ӷ�
            auto& res = cluster_msg.handshake_res();
            if (node_session->info().node_name != res.target_node()) {
                LOG_ERR("Target node name mismatch, ip: {}, port: {}, target_node: {}, res_target_node: {}", ip, port, node_session->info().node_name, res.target_node());
                msg->connection->Close();
                co_return;
            }
            
            // �����ڵ�
            auto& node_session_ref = CreateNodeSession(std::move(msg->connection), true);

            // ������ӣ��������ж��а�
            for (auto iter = send_queue_.begin(); iter != send_queue_.end(); ) {
                auto& msg = *iter;
                if (msg->target_node == res.target_node()) {
                    ForwardPacket(&node_session_ref, std::move(msg));
                    send_queue_.erase(iter++);
                }
                else {
                    ++iter;
                }
            }
            wait_nodes_.erase(res.target_node());
            break;
        }
        case Ss::Cluster::ClusterMsg::BodyCase::kForwardHeader: {
            // ����Ҫ�ж��£�����û��������֣�������ת����

            auto& header = cluster_msg.forward_header();
            auto& src_service = header.src_service();
            auto& target_service = header.target_service();
            auto target_service_handle = imillion_->GetServiceByUniqueNum(target_service);
            if (target_service_handle) {
                // ����Ҫ��ȡ��Դ�ڵ�
                auto span = net::PacketSpan(msg->packet.begin() + sizeof(header_size) + header_size, msg->packet.end());
                Send<ClusterRecvPacketMsg>(*target_service_handle, 
                    NodeSessionHandle{ .src_node = node_session->info().node_name, .src_service = src_service},
                    std::move(msg->packet), span);
            }
            break;
        }
        }
        co_return;
    }

    MILLION_MSG_HANDLE(ClusterSendPacketMsg, msg) {
        EndPointRes end_point;
        auto node_session = FindNodeSession(msg->target_node, &end_point);
        if (!node_session && end_point.ip.empty()) {
            LOG_ERR("Node cannot be found: {}.", msg->target_node);
            co_return;
        }
        if (node_session) {
            ForwardPacket(node_session, std::move(msg));
            co_return;
        }

        if (wait_nodes_.find(msg->target_node) == wait_nodes_.end()) {
            wait_nodes_.emplace(msg->target_node);
            auto& io_context = imillion_->NextIoContext();
            asio::co_spawn(io_context.get_executor(), [this, target_node = msg->target_node, end_point = std::move(end_point)]() mutable -> asio::awaitable<void> {
                auto connection_opt = co_await server_.ConnectTo(end_point.ip, end_point.port);
                if (!connection_opt) {
                    LOG_ERR("server_.ConnectTo failed, ip: {}, port: {}.", end_point.ip, end_point.port);
                    co_return;
                }

                // ����
                auto connection = *connection_opt;
                auto node_session = connection->get_ptr<NodeSession>();

                Ss::Cluster::ClusterMsg cluster_msg;
                auto* req = cluster_msg.mutable_handshake_req();
                req->set_src_node(node_name_);
                req->set_target_node(target_node);
                auto packet = ProtoMsgToPacket(cluster_msg);

                SendInit(node_session, packet, net::Packet());

                auto span = net::PacketSpan(packet.begin(), packet.end());
                node_session->Send(std::move(packet), span, 0);

                node_session->info().node_name = std::move(target_node);
                }, asio::detached);
        }

        // ������������״̬����Ҫsend�����а��ŵ���������������ʱͳһ����
        // ʹ��һ��ȫ�ֶ��У���Ϊ���������Ŷӵİ����٣�ֱ��ɨ�����
        send_queue_.emplace_back(std::move(msg));

        co_return;
    }

private:
    NodeSession& CreateNodeSession(net::TcpConnectionShared&& connection, bool active) {
        // �����������Ҫ����ʧ��
        auto node_session = connection->get_ptr<NodeSession>();
        auto& target_node_name = node_session->info().node_name;
        auto res = nodes_.emplace(target_node_name, connection);
        if (res.second) {
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
        return *res.first->second->get_ptr<NodeSession>();
    }

    struct EndPointRes {
        std::string ip;
        std::string port;
    };
    NodeSession* FindNodeSession(NodeUniqueName node_name, EndPointRes* end_point) {
        auto iter = nodes_.find(node_name);
        if (iter != nodes_.end()) {
            return iter->second->get_ptr<NodeSession>();
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

        if (end_point) {
            for (auto node : nodes) {
                auto node_name = node.first.as<std::string>();
                if (node_name != node_name) continue;

                end_point->ip = node.second["ip"].as<std::string>();
                end_point->port = node.second["port"].as<std::string>();

                break;
            }
        }
        return nullptr;
    }

    void DeleteNodeSession(NodeSession* node_session) {
        nodes_.erase(node_session->info().node_name);
    }

    void SendInit(NodeSession* node_session, const net::Packet& header_packet, const net::Packet& forward_packet) {
        uint32_t header_size = header_packet.size();
        header_size = asio::detail::socket_ops::host_to_network_long(header_size);
        auto size_packet = net::Packet(sizeof(header_size));
        std::memcpy(size_packet.data(), &header_size, sizeof(header_size));
        auto size_span = net::PacketSpan(size_packet.begin(), size_packet.end());
        auto total_size = sizeof(header_size) + header_packet.size() + forward_packet.size();
        node_session->Send(std::move(size_packet), size_span, total_size);
    }

    void ForwardPacket(NodeSession* node_session, std::unique_ptr<ClusterSendPacketMsg> msg) {
        // ׷�Ӽ�Ⱥͷ��
        Ss::Cluster::ClusterMsg cluster_msg;
        auto* header = cluster_msg.mutable_forward_header();
        header->set_src_service(std::move(msg->src_service));
        header->set_target_service(std::move(msg->target_service));
        auto header_packet = ProtoMsgToPacket(cluster_msg);
        
        SendInit(node_session, header_packet, msg->packet);

        auto header_span = net::PacketSpan(header_packet.begin(), header_packet.end());
        node_session->Send(std::move(header_packet), header_span, 0);

        auto span = net::PacketSpan(msg->packet.begin(), msg->packet.end());
        node_session->Send(std::move(msg->packet), span, 0);
    }

private:
    ClusterServer server_;
    NodeUniqueName node_name_;
    std::unordered_map<NodeUniqueName, net::TcpConnectionShared> nodes_;

    std::set<std::string> wait_nodes_;
    std::list<std::unique_ptr<ClusterSendPacketMsg>> send_queue_;
};

} // namespace gateway
} // namespace million
