#pragma once

#include <million/detail/noncopyable.h>

#include "token.h"
#include "net/tcp_connection_handle.h"

namespace million {
namespace gateway {

struct UserHeader {
    Token token;
};

class ProtoMgr;
class UserSession : noncopyable {
public:
    UserSession(ProtoMgr* proto_mgr, net::TcpConnectionShared&& connection, const Token& token);
    ~UserSession();

    bool Send(const protobuf::Message& message);

    const UserHeader& header() const { return header_; }
    void set_header(const UserHeader& header) { header_ = header; }

private:
    ProtoMgr* proto_mgr_;
    net::TcpConnectionShared connection_;
    UserHeader header_;
};

} // namespace gateway
} // namespace million
