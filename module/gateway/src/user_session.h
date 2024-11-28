#pragma once

#include <google/protobuf/message.h>

#include <million/noncopyable.h>
#include <million/net/tcp_connection.h>
#include <million/proto_msg.h>

#include <gateway/gateway_msg.h>

#include "token.h"

namespace million {
namespace gateway {

struct UserSessionInfo {
    Token token = kInvaildToken;
    GatewayContextId user_context_id = 0;
};

class UserSession : public net::TcpConnection {
public:
    UserSession(net::TcpServer* server, asio::ip::tcp::socket&& socket, asio::any_io_executor&& executor);
    ~UserSession();

    const UserSessionInfo& info() const { return info_; }
    UserSessionInfo& info() { return info_; }

private:
    UserSessionInfo info_;
};

} // namespace gateway
} // namespace million
