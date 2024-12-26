#pragma once

#include <google/protobuf/message.h>

#include <million/noncopyable.h>
#include <million/net/tcp_connection.h>
#include <million/msg.h>

#include <cluster/cluster.h>

namespace million {
namespace cluster {

struct NodeSessionInfo {
    NodeName node_name;
    
    // 正在等待远程节点的消息的会话，NodeServiceSessionId表示一条虚拟会话，映射两个节点之间的服务之间的session的id
    std::unordered_map<ServiceName, std::vector<NodeServiceSessionId>> services_;
};

class NodeSession : public net::TcpConnection {
public:
    NodeSession(net::TcpServer* server, asio::ip::tcp::socket&& socket, asio::any_io_executor&& executor);
    ~NodeSession();

    const NodeSessionInfo& info() const { return info_; }
    NodeSessionInfo& info() { return info_; }

private:
    NodeSessionInfo info_;
};

} // namespace cluster
} // namespace million
