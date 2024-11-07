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

TcpConnectionShared TcpServer::MakeTcpConnectionShared(TcpServer* server, asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor) const {
    return std::make_shared<TcpConnection>(server, std::move(socket), executor);
}

TcpConnectionShared TcpServer::AddConnection(asio::ip::tcp::socket&& socket, const asio::any_io_executor& executor) {
    decltype(connections_)::iterator iter;
    auto connection = MakeTcpConnectionShared(this, std::move(socket), executor);
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
        asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
        // 获取io_context，新连接绑定到该io_context中
        auto& io_context = imillion_->NextIoContext();
        auto handle = AddConnection(std::move(socket), io_context.get_executor());
        if (on_connection_) {
            on_connection_(handle);
        }
        handle->Process();
    }
}

} // namespace net
} // namespace million