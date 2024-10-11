#pragma once

#include <memory>
#include <thread>

#include "milinet/detail/noncopyable.h"

namespace milinet {

class Milinet;
class Worker : noncopyable {
public:
    Worker(Milinet* milinet);
    ~Worker();

    void Start();

private:
    Milinet* milinet_;
    std::unique_ptr<std::jthread> thread_;
};

} // namespace milinet