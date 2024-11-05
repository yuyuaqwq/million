#pragma once

#include <google/protobuf/message.h>

#include <million/noncopyable.h>

#include "token.h"
#include "net/tcp_connection.h"

namespace million {
namespace gateway {

struct UserHeader {
    Token token;
};

class CsProtoMgr;
class UserSession : public net::TcpConnection {
public:
    UserSession(CsProtoMgr* proto_mgr, net::TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor);
    ~UserSession();

    bool Send(const google::protobuf::Message& message);

    const UserHeader& header() const { return header_; }
    UserHeader& header() { return header_; }

private:
    CsProtoMgr* proto_mgr_;
    UserHeader header_;
};

} // namespace gateway
} // namespace million
