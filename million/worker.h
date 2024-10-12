#pragma once

#include <memory>
#include <thread>

#include "million/detail/noncopyable.h"

namespace million {

class million;
class Worker : noncopyable {
public:
    Worker(million* million);
    ~Worker();

    void Start();
    void Join();
    void Detach();

private:
    million* million_;
    std::unique_ptr<std::thread> thread_;
};

} // namespace million