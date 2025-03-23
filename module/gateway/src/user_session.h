#pragma once

#include <google/protobuf/message.h>

#include <million/noncopyable.h>
#include <million/net/tcp_connection.h>
#include <million/msg.h>

#include <gateway/gateway.h>

#include "token.h"

namespace million {
namespace gateway {

//struct UserSessionInfo {
//    Token token = kInvaildToken;
//    million::SessionId agent_id = kSessionIdInvalid;
//    ServiceHandle agent_;
//};

class UserSession : public net::TcpConnection {
public:
    using Base = net::TcpConnection;
    using Base::Base;

    const Token& token() const { return token_; }
    void set_token(const Token& token) { token_ = token; }

    SessionId agent_id() const { return agent_id_; }
    void set_agent_id(SessionId agent_id) { agent_id_ = agent_id; }

    const ServiceHandle& agent() const { return agent_; }
    void set_agent(const ServiceHandle& agent) { agent_ = agent; }

private:
    Token token_ = kInvaildToken;
    million::SessionId agent_id_ = kSessionIdInvalid;    // agent_id
    ServiceHandle agent_;
};

} // namespace gateway
} // namespace million
