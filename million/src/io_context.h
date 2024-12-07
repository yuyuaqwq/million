#pragma once

#include <memory>
#include <thread>
#include <optional>

#include <asio/io_context.hpp>

#include <million/noncopyable.h>

namespace million {

class Million;
class IoContext : noncopyable {
public:
    IoContext(Million* million);
    ~IoContext();

    void Start();
    void Stop();

    auto& io_context() { return io_context_; }

private:
    Million* million_;
    std::optional<std::jthread> thread_;
    asio::io_context io_context_;
    std::optional<asio::io_context::work> work_;
};

} // namespace million