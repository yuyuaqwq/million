#pragma once

#include <queue>
#include <mutex>
#include <list>
#include <optional>

#include <asio.hpp>

#include "million/detail/noncopyable.h"
#include "net/connection_handle.h"
#include "net/packet.h"

namespace million {
namespace net {

class Server : noncopyable {
public:
    using ConnectionFunc = std::function<void(ConnectionHandle)>;
    using MsgFunc = std::function<void(Connection&, Packet&&)>;

public:
    Server();
    ~Server();

    auto& on_connection() const { return on_connection_; }
    void set_on_connection(const ConnectionFunc& on_connection) { on_connection_ = on_connection; }
    auto& on_msg() const { return on_msg_; }
    void set_on_msg(const MsgFunc& on_msg) { on_msg_ = on_msg; }


    void Start(size_t io_thread_num, uint16_t port);
    void Stop();
    void RemoveConnection(std::list<std::shared_ptr<Connection>>::iterator iter);

private:
    ConnectionHandle AddConnection(asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor);
    asio::awaitable<void> Listen(uint16_t port);

private:
    std::optional<asio::io_context::work> work_;
    asio::io_context io_context_;

    std::vector<std::thread> io_threads_;

    std::mutex connections_mutex_;
    std::list<std::shared_ptr<Connection>> connections_;

    ConnectionFunc on_connection_;
    MsgFunc on_msg_;
};

} // namespace net
} // namespace million