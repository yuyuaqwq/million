#pragma once

#include <google/protobuf/message.h>

#include <million/noncopyable.h>
#include <million/net/tcp_connection.h>
#include <million/msg.h>

#include <cluster/cluster.h>

namespace million {
namespace cluster {

struct NodeSessionInfo {
    NodeContextId node_context_id;
    NodeName node_name;
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
