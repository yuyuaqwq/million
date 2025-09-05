#pragma once

#include <google/protobuf/message.h>

#include <million/imillion.h>
#include <million/net/tcp_connection.h>

#include <gateway/gateway.h>

#include "token.h"

namespace million {
namespace gateway {

//struct UserSessionInfo {
//    Token token = kInvaildToken;
//    million::SessionId agent_id = kSessionIdInvalid;
//    ServiceHandle agent_handle;
//};

class UserSession : public net::TcpConnection {
public:
    using Base = net::TcpConnection;
    using Base::Base;

    const Token& token() const { return token_; }
    void set_token(const Token& token) { token_ = token; }

    SessionId agent_id() const { return agent_id_; }
    void set_agent_id(SessionId agent_id) { agent_id_ = agent_id; }

    const ServiceHandle& agent_handle() const { return agent_handle_; }
    void set_agent_handle(const ServiceHandle& agent_handle) { agent_handle_ = agent_handle; }

private:
    Token token_ = kInvaildToken;
    million::SessionId agent_id_ = kSessionInvalidId;    // agent_id
    ServiceHandle agent_handle_;
};

} // namespace gateway
} // namespace million
