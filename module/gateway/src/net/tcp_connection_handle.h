#pragma once

#include <memory>

namespace million {
namespace net {

class TcpConnection;
using TcpConnectionShared = std::shared_ptr<TcpConnection>;

//class TcpConnection;
//class TcpConnectionHandle {
//public:
//    explicit TcpConnectionHandle(std::shared_ptr<TcpConnection> connection)
//        : connection_(std::move(connection)) {}
//    ~TcpConnectionHandle() = default;
//
//    TcpConnectionHandle(const TcpConnectionHandle&) = default;
//    void operator=(const TcpConnectionHandle& v) {
//        connection_ = v.connection_;
//    }
//
//    TcpConnection& connection() const { return *connection_; }
//
//private:
//    std::shared_ptr<TcpConnection> connection_;
//};

} // namespace net
} // namespace million



