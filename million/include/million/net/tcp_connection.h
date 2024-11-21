#pragma once

#include <list>
#include <queue>
#include <mutex>

#include <asio.hpp>

#include <million/api.h>
#include <million/net/packet.h>
#include <million/net/tcp_connection_handle.h>

namespace million {
namespace net {

class TcpServer;
class MILLION_CLASS_API TcpConnection {
public:
    TcpConnection(TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor);
    virtual ~TcpConnection();

    void Close();
    void Process();
    void Send(Packet&& packet);
    void Send(Packet&& packet, std::span<uint8_t> span, uint32_t total_size);
    bool Connected();

    auto iter() const { return iter_; }
    void set_iter(std::list<TcpConnectionShared>::iterator iter) { iter_ = iter; }

    asio::ip::tcp::socket& socket() { return socket_; }

public:
    TcpServer* server_;
    std::list<TcpConnectionShared>::iterator iter_;

    std::mutex close_mutex_;

    asio::ip::tcp::socket socket_;
    const asio::any_io_executor& executor_;

    std::mutex send_queue_mutex_;
    struct SendPacket {
        Packet packet;
        std::span<uint8_t> span;
        uint32_t total_size;  // 为0表示不写入total_size
    };
    std::queue<SendPacket> send_queue_;
};

} // namespace net
} // namespace million