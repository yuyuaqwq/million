#include <million/net/tcp_server.h>
#include <million/net/tcp_connection.h>

namespace million {
namespace net {

TcpServer::TcpServer(IMillion* imillion)
    : imillion_(imillion) {}
TcpServer::~TcpServer() = default;

void TcpServer::Start(uint16_t port) {
    // 获取一个io_context，绑定acceptor
    auto& io_context = imillion_->NextIoContext();
    asio::co_spawn(io_context, Listen(port), asio::detached);
}

void TcpServer::Stop() {
    {
        auto lock = std::lock_guard(connections_mutex_);
        for (auto& connection : connections_) {
            connection->Close();
        }
    }
}

void TcpServer::RemoveConnection(std::list<TcpConnectionShared>::iterator iter) {
    auto lock = std::lock_guard(connections_mutex_);
    connections_.erase(iter);
}

asio::awaitable<std::optional<TcpConnectionShared>> TcpServer::ConnectTo(std::string_view host, std::string_view port) {
    // 获取io_context，新连接绑定到该io_context中
    try {
        auto executor = co_await asio::this_coro::executor;
        auto resolver = asio::ip::tcp::resolver(executor);
        auto endpoints = co_await resolver.async_resolve(host, port, asio::use_awaitable);
        auto socket = asio::ip::tcp::socket(executor);
        co_await asio::async_connect(socket, endpoints, asio::use_awaitable);
        auto& io_context = imillion_->NextIoContext();
        auto handle = AddConnection(std::move(socket), io_context.get_executor());
        handle->Process(true);
        co_return handle;
    }
    catch (const std::exception& e) {
        imillion().logger().LOG_ERROR("Tcp server ConnectTo exception: {}", e.what());
    }
    co_return std::nullopt;
}

TcpConnectionShared TcpServer::MakeTcpConnectionShared(TcpServer* server, asio::ip::tcp::socket&& socket, asio::any_io_executor&& executor) const {
    return std::make_shared<TcpConnection>(server, std::move(socket), std::move(executor));
}

TcpConnectionShared TcpServer::AddConnection(asio::ip::tcp::socket&& socket, asio::any_io_executor&& executor) {
    decltype(connections_)::iterator iter;
    auto connection = MakeTcpConnectionShared(this, std::move(socket), std::move(executor));
    auto handle = TcpConnectionShared(connection);
    {
        auto lock = std::lock_guard(connections_mutex_);
        connections_.emplace_back(std::move(connection));
        iter = --connections_.end();
    }
    handle->set_iter(iter);
    return handle;
}

asio::awaitable<void> TcpServer::Listen(uint16_t port) {
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor(executor, { asio::ip::tcp::v4(), port });
    while (true) {
        try {
            asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
            // 获取io_context，新连接绑定到该io_context中
            auto& io_context = imillion_->NextIoContext();
            auto handle = AddConnection(std::move(socket), io_context.get_executor());
            handle->Process(true);
        }
        catch (const std::exception& e) {
            imillion().logger().LOG_ERROR("Tcp server Listen exception: {}", e.what());
        }
    }
    co_return;
}

} // namespace net
} // namespace million