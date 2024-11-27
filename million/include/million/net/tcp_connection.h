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
class MILLION_CLASS_API TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(TcpServer* server, asio::ip::tcp::socket&& socket, asio::any_io_executor&& executor);
    virtual ~TcpConnection();

    void Close();
    void Process(bool call_on_connection);
    void Send(Packet&& packet);
    void Send(Packet&& packet, PacketSpan span, uint32_t total_size);
    bool Connected() const;

    template<typename T>
    T* get_ptr() { return static_cast<T*>(this); }

    auto iter() const { return iter_; }
    void set_iter(std::list<TcpConnectionShared>::iterator iter) { iter_ = iter; }

    asio::ip::tcp::socket& socket() { return socket_; }
    const asio::ip::tcp::socket& socket() const { return socket_; }
    const asio::ip::tcp::endpoint& remote_endpoint() const { return remote_endpoint_; }

public:
    TcpServer* server_;
    std::list<TcpConnectionShared>::iterator iter_;

    std::mutex close_mutex_;

    asio::ip::tcp::socket socket_;
    asio::ip::tcp::endpoint remote_endpoint_;
    asio::any_io_executor executor_;

    std::mutex send_queue_mutex_;
    struct SendPacket {
        Packet packet;
        PacketSpan span;
        uint32_t total_size;  // 为0表示不写入total_size
    };
    bool sending_ = false;
    std::queue<SendPacket> send_queue_;
};

} // namespace net
} // namespace million