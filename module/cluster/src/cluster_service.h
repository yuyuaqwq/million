#pragma once

#include <million/iservice.h>

#include <protogen/ss/ss_cluster.pb.h>

#include <cluster/cluster_msg.h>

#include "cluster_server.h"

namespace million {
namespace cluster {

namespace Ss = ::Million::Proto::Ss;

MILLION_MSG_DEFINE(, ClusterTcpConnectionMsg, (net::TcpConnectionShared)connection)
MILLION_MSG_DEFINE(, ClusterTcpRecvPacketMsg, (net::TcpConnectionShared)connection, (net::Packet)packet)

class ClusterService : public IService {
public:
    using Base = IService;
    ClusterService(IMillion* imillion)
        : Base(imillion)
        , server_(imillion) { }

    virtual bool OnInit() override {
        // io�̻߳ص�������work�̴߳���
        server_.set_on_connection([this](auto&& connection) {
            Send<ClusterTcpConnectionMsg>(service_handle(), connection);
        });
        server_.set_on_msg([this](auto&& connection, auto&& packet) {
            Send<ClusterTcpRecvPacketMsg>(service_handle(), connection, std::move(packet));
        });

        const auto& config = imillion_->YamlConfig();
        const auto& cluster_config = config["cluster"];
        if (!cluster_config) {
            std::cerr << "[cluster] [config] [error] cannot find 'cluster'." << std::endl;
            return false;
        }

        const auto& name_config = cluster_config["name"];
        if (!name_config)
        {
            std::cerr << "[cluster] [config] [error] cannot find 'cluster.name'." << std::endl;
            return false;
        }
        node_name_ = name_config.as<std::string>();

        const auto& port_config = cluster_config["port"];
        if (!port_config)
        {
            std::cerr << "[cluster] [config] [error] cannot find 'cluster.port'." << std::endl;
            return false;
        }
        server_.Start(port_config.as<uint16_t>());
        return true;
    }

    MILLION_MSG_DISPATCH(ClusterService);

    MILLION_MSG_HANDLE(ClusterTcpConnectionMsg, msg) {
        // �п���Ŀ��ڵ��Ѿ��������ӵ�ǰ�ڵ�
        co_return;
    }

    MILLION_MSG_HANDLE(ClusterTcpRecvPacketMsg, msg) {
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
            co_return;
        }
        if (!connection_ptr) {
            auto& io_context = imillion_->NextIoContext();
            // ����Ҫ��Ǹýڵ�Ϊ�����У����ӹ����������������ýڵ����Ϣ��Ҫ������еȴ�
            asio::co_spawn(io_context.get_executor(), [this, msg = std::move(msg), end_point = std::move(end_point)]() mutable -> asio::awaitable<void> {
                auto res = co_await server_.ConnectTo(end_point.ip, end_point.port);
                if (!res) {
                    co_return;
                }
                // io�߳�ִ�У����´���������Ϣ
                auto imsg = msg.get();
                Resend(service_handle(), std::move(msg));
            }, asio::detached);
            co_return;
        }
        auto& connection = *connection_ptr;
        // ׷�Ӽ�Ⱥͷ��
        Ss::ClusterHeader header;
        header.set_src_service(std::move(msg->src_service));
        header.set_target_service(std::move(msg->target_service));
        auto header_packet = net::Packet(header.ByteSize());
        connection->Send(std::move(header_packet), net::PacketSpan(header_packet.begin(), header_packet.end()), header_packet.size() + msg->packet.size());
        connection->Send(std::move(msg->packet), net::PacketSpan(msg->packet.begin(), msg->packet.end()), 0);
        co_return;
    }

private:
    struct EndPointRes {
        std::string ip;
        std::string port;
    };
    net::TcpConnectionShared* FindNode(NodeUniqueName node_name, EndPointRes* end_point) {
        auto iter = nodes_.find(node_name);
        if (iter != nodes_.end()) {
            return &iter->second;
        }

        // ���Դ������ļ��в��Ҵ˽ڵ�
        const auto& config = imillion_->YamlConfig();
        const auto& cluster_config = config["cluster"];
        if (!cluster_config) {
            std::cerr << "[cluster] [config] [error] cannot find 'cluster'." << std::endl;
            return nullptr;
        }

        const auto& nodes = cluster_config["nodes"];
        if (!nodes) {
            std::cerr << "[cluster] [config] [error] cannot find 'cluster.nodes'." << std::endl;
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
