#pragma once

#include <memory>
#include <thread>
#include <optional>

#include <million/noncopyable.h>

namespace million {

class Million;
class Worker : noncopyable {
public:
    Worker(Million* million);
    ~Worker();

    void Start();
    void Stop();

private:
    Million* million_;
    std::optional<std::jthread> thread_;
    bool run_ = false;
};

} // namespace million