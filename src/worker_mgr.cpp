#include "milinet/worker_mgr.h"

#include "milinet/worker.h"

namespace milinet {

WorkerMgr::WorkerMgr(Milinet* milinet, size_t worker_num)
    : milinet_(milinet) {
    if (worker_num == 0) {
        worker_num = std::thread::hardware_concurrency();
    }
    workers_.reserve(worker_num);
    for (size_t i = 0; i < worker_num; ++i) {
        workers_.emplace_back(std::make_unique<Worker>(milinet_));
    }
}

WorkerMgr::~WorkerMgr() = default;

void WorkerMgr::Start() {
    for (auto& worker : workers_) {
        worker->Start();
    }
}

} // namespace milinet