#pragma once

#include <google/protobuf/message.h>

#include <million/detail/noncopyable.h>

#include "token.h"
#include "net/tcp_connection.h"

namespace million {
namespace gateway {

struct UserHeader {
    Token token;
};

class ProtoMgr;
class UserSession : public net::TcpConnection {
public:
    UserSession(ProtoMgr* proto_mgr, net::TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor);
    ~UserSession();

    bool Send(const google::protobuf::Message& message);

    const UserHeader& header() const { return header_; }
    UserHeader& header() { return header_; }

private:
    ProtoMgr* proto_mgr_;
    UserHeader header_;
};

} // namespace gateway
} // namespace million
