#pragma once

#include <google/protobuf/message.h>

#include <million/noncopyable.h>
#include <million/net/tcp_connection.h>

#include "token.h"

namespace million {
namespace gateway {

struct UserHeader {
    Token token;
};

template<typename HeaderT>
class CsProtoMgr;

class UserSession : public net::TcpConnection {
public:
    UserSession(CsProtoMgr<UserHeader>* proto_mgr, net::TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor);
    ~UserSession();

    bool Send(const google::protobuf::Message& message);

    const UserHeader& header() const { return header_; }
    UserHeader& header() { return header_; }

private:
    CsProtoMgr<UserHeader>* proto_mgr_;
    UserHeader header_;
};

} // namespace gateway
} // namespace million
