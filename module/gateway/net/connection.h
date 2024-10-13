#pragma once

#include <queue>
#include <mutex>
#include <list>

#include <asio.hpp>

#include "net/packet.h"

namespace million {
namespace net {

class Server;
class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(Server* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor);
    ~Connection();

    void Close();

    void Process();

    void Send(Packet&& packet);

    auto iter() const { return iter_; }
    void set_iter(std::list<std::shared_ptr<Connection>>::iterator iter) { iter_ = iter; }

    asio::ip::tcp::socket& socket() { return socket_; }

public:
    Server* server_;
    std::list<std::shared_ptr<Connection>>::iterator iter_;

    std::mutex close_mutex_;

    asio::ip::tcp::socket socket_;
    const asio::any_io_executor& executor_;

    std::mutex send_queue_mutex_;
    std::queue<Packet> send_queue_;
};

} // namespace net
} // namespace million