#pragma once

#include <google/protobuf/message.h>

#include <million/noncopyable.h>
#include <million/net/tcp_connection.h>
#include <million/msg.h>

#include <cluster/cluster.h>

namespace million {
namespace cluster {

class NodeSession : public net::TcpConnection {
public:
    using Base = net::TcpConnection;
    using Base::Base;

    const NodeName& node_name() const { return node_name_; }
    void set_node_name(const NodeName& node_name) { node_name_ = node_name; }

    SessionId node_session_id() const { return node_session_id_; }
    void set_node_session_id(SessionId node_session_id) { node_session_id_ = node_session_id; }

private:
    NodeName node_name_;
    SessionId node_session_id_ = kSessionIdInvalid;
};

} // namespace cluster
} // namespace million
