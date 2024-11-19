#pragma once

#include <google/protobuf/message.h>

#include <million/noncopyable.h>
#include <million/net/tcp_connection.h>

#include "token.h"

namespace million {
namespace cluster {

struct UserHeader {
    Token token;
};

class UserSession : public net::TcpConnection {
public:
    UserSession(net::TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor);
    ~UserSession();

    const UserHeader& header() const { return header_; }
    UserHeader& header() { return header_; }

private:
    UserHeader header_;
};

} // namespace gateway
} // namespace million
