#pragma once

#include <memory>

namespace milinet {
namespace net {

class Connection;
class ConnectionHandle {
public:
    explicit ConnectionHandle(std::shared_ptr<Connection> connection)
        : connection_(std::move(connection)) {}
    ~ConnectionHandle() = default;

    ConnectionHandle(const ConnectionHandle&) = default;
    void operator=(const ConnectionHandle& v) {
        connection_ = v.connection_;
    }

    Connection& connection() const { return *connection_; }

private:
    std::shared_ptr<Connection> connection_;
};

} // namespace net
} // namespace milinet



