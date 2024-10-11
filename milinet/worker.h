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
    void Join();
    void Detach();

private:
    Milinet* milinet_;
    std::unique_ptr<std::thread> thread_;
};

} // namespace milinet