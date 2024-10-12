#pragma once

#include <queue>
#include <mutex>
#include <list>
#include <optional>

#include <asio.hpp>

#include "million/detail/noncopyable.h"
#include "million/net/connection_handle.h"

namespace million {
namespace net {

class Server : noncopyable {
public:
    Server();
    ~Server();

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
};

} // namespace net
} // namespace million