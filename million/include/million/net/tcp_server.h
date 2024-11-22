#pragma once

#include <queue>
#include <mutex>
#include <list>
#include <optional>

#include <asio.hpp>

#include <million/noncopyable.h>
#include <million/imillion.h>
#include <million/net/packet.h>
#include <million/net/tcp_connection.h>

namespace million {
namespace net {

class MILLION_CLASS_API TcpServer : noncopyable {
public:
    using TcpConnectionFunc = std::function<asio::awaitable<void>(const TcpConnectionShared&)>;
    using TcpMsgFunc = std::function<asio::awaitable<void>(const TcpConnectionShared&, Packet&&)>;

public:
    TcpServer(IMillion* imillion);
    virtual ~TcpServer();

    auto& on_connection() const { return on_connection_; }
    void set_on_connection(const TcpConnectionFunc& on_connection) { on_connection_ = on_connection; }
    auto& on_msg() const { return on_msg_; }
    void set_on_msg(const TcpMsgFunc& on_msg) { on_msg_ = on_msg; }

    void Start(uint16_t port);
    void Stop();
    void RemoveConnection(std::list<TcpConnectionShared>::iterator iter);
    asio::awaitable<std::optional<TcpConnectionShared>> ConnectTo(std::string_view host, std::string_view port);

    virtual TcpConnectionShared MakeTcpConnectionShared(TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor) const;

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