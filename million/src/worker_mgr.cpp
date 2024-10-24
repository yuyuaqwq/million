#include "worker_mgr.h"

#include "worker.h"

namespace million {

WorkerMgr::WorkerMgr(Million* million, size_t worker_num)
    : million_(million) {
    if (worker_num == 0) {
        worker_num = std::thread::hardware_concurrency();
    }
    workers_.reserve(worker_num);
    for (size_t i = 0; i < worker_num; ++i) {
        workers_.emplace_back(std::make_unique<Worker>(million_));
    }
}

WorkerMgr::~WorkerMgr() = default;

void WorkerMgr::Start() {
    for (auto& worker : workers_) {
        worker->Start();
    }
}

void WorkerMgr::Stop() {
    for (auto& worker : workers_) {
        worker->Stop();
    }
}

} // namespace million