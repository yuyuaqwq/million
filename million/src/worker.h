#pragma once

#include <memory>
#include <thread>
#include <optional>

#include <million/detail/noncopyable.h>

namespace million {

class Million;
class Worker : noncopyable {
public:
    Worker(Million* million);
    ~Worker();

    void Start();

private:
    Million* million_;
    std::optional<std::jthread> thread_;
};

} // namespace million