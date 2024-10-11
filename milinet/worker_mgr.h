#pragma once

#include <memory>
#include <vector>

#include "milinet/detail/noncopyable.h"

namespace milinet {

class Milinet;
class Worker;
class WorkerMgr : noncopyable {
public:
    WorkerMgr(Milinet* milinet, size_t worker_num);
    ~WorkerMgr();

    void Start();

private:
    Milinet* milinet_;
    std::vector<std::unique_ptr<Worker>> workers_;
};

} // namespace milinet