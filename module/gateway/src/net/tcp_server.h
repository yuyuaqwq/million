#pragma once

#include <queue>
#include <mutex>
#include <list>
#include <optional>

#include <asio.hpp>

#include <million/detail/noncopyable.h>
#include <million/imillion.h>

#include "net/tcp_connection_handle.h"
#include "net/packet.h"

namespace million {

namespace net {

class TcpServer : noncopyable {
public:
    using TcpConnectionFunc = std::function<void(TcpConnectionShared)>;
    using TcpMsgFunc = std::function<void(TcpConnection&, Packet&&)>;

public:
    TcpServer(IMillion* imillion);
    ~TcpServer();

    auto& on_connection() const { return on_connection_; }
    void set_on_connection(const TcpConnectionFunc& on_connection) { on_connection_ = on_connection; }
    auto& on_msg() const { return on_msg_; }
    void set_on_msg(const TcpMsgFunc& on_msg) { on_msg_ = on_msg; }

    void Start(uint16_t port);
    void Stop();
    void RemoveConnection(std::list<std::shared_ptr<TcpConnection>>::iterator iter);

private:
    TcpConnectionShared AddConnection(asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor);
    asio::awaitable<void> Listen(uint16_t port);

private:
    IMillion* imillion_;

    std::mutex connections_mutex_;
    std::list<TcpConnectionShared> connections_;

    TcpConnectionFunc on_connection_;
    TcpMsgFunc on_msg_;
};

} // namespace net
} // namespace million