#pragma once

#include <memory>

namespace million {
namespace net {

class TcpConnection;
using TcpConnectionShared = std::shared_ptr<TcpConnection>;

} // namespace net
} // namespace million
