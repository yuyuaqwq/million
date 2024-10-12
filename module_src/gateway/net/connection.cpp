#include "million/net/connection.h"

#include "million/net/server.h"

namespace million {
namespace net {

Connection::Connection(
    Server* server,
    asio::ip::tcp::socket&& socket,
    const asio::any_io_executor& executor)
    : server_(server)
    , socket_(std::move(socket))
    , executor_(executor) {};

Connection::~Connection() = default;

void Connection::Close() {
    std::lock_guard guard(close_mutex_);
    if (!socket_.is_open()) {
        return;
    }
    asio::error_code ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
}

void Connection::Process() {
    asio::co_spawn(executor_, [this]() -> asio::awaitable<void> {
        try {
            while (true) {
                uint32_t total_packet_size = 0;
                uint32_t bytes_read = co_await asio::async_read(
                    socket_
                    , asio::buffer(&total_packet_size, sizeof(total_packet_size))
                    , asio::use_awaitable);
                if (bytes_read != sizeof(total_packet_size)) {
                    break;
                }
                total_packet_size = asio::detail::socket_ops::network_to_host_long(total_packet_size);

                // �ֶζ�ȡ���������total_packet_size
                constexpr uint32_t max_read_size = 65536u;
                constexpr uint32_t max_expansion_size = 1024 * 1024 * 64;
                auto packet = Packet(std::min(total_packet_size, max_read_size));
                uint32_t total_bytes_read = 0;
                while (total_bytes_read < total_packet_size) {
                    uint32_t chunk_size = std::min(total_packet_size - total_bytes_read, max_read_size);
                    uint32_t new_size = packet.size();
                    while (new_size < total_bytes_read + chunk_size) {
                        if (new_size < max_expansion_size) {
                            new_size *= 2;
                        }
                        else {
                            new_size += max_expansion_size;
                        }
                    }
                    packet.resize(new_size);
                    bytes_read = co_await asio::async_read(
                        socket_,
                        asio::buffer(packet.data() + total_bytes_read, chunk_size),
                        asio::use_awaitable);
                    if (bytes_read != chunk_size) {
                        break;
                    }
                    total_bytes_read += bytes_read;
                }
            }
        }
        catch (std::exception& e) {
            Close();
            server_->RemoveConnection(iter_);
        }
    }, asio::detached);
}

void Connection::Send(Packet&& packet) {
    {
        auto guard = std::lock_guard(send_queue_mutex_);
        bool send_in_progress = !send_queue_.empty();
        send_queue_.emplace(std::move(packet));
        if (send_in_progress) {
            return;
        }
    }
    // ����Ҫ���͵İ�����һ��Э��ȥ����������Ϣ
    asio::co_spawn(executor_, [this]() -> asio::awaitable<void> {
        std::queue<Packet> tmp_queue;
        try {
            do {
                {
                    auto guard = std::lock_guard(send_queue_mutex_);
                    if (send_queue_.empty()) break;
                    std::swap(tmp_queue, send_queue_);
                }
                while (!tmp_queue.empty()) {
                    auto& packet = tmp_queue.front();
                    uint32_t len = packet.size();
                    len = asio::detail::socket_ops::host_to_network_long(len);
                    co_await asio::async_write(socket_, asio::buffer(&len, sizeof(len)), asio::use_awaitable);
                    co_await asio::async_write(socket_, asio::buffer(packet.data(), packet.size()), asio::use_awaitable);
                    tmp_queue.pop();
                }
            } while (true);
        }
        catch (const std::exception& e) {
            Close();
        }
    });
}

} // namespace net
} // namespace million