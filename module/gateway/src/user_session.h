#pragma once

#include <google/protobuf/message.h>

#include <million/noncopyable.h>
#include <million/net/tcp_connection.h>
#include <million/msg.h>

#include <gateway/gateway.h>

#include "token.h"

namespace million {
namespace gateway {

struct UserSessionInfo {
    Token token = kInvaildToken;
    million::SessionId persistent_session_id = 0;
    ServiceHandle agent_;
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
