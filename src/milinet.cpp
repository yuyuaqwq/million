#include "milinet/milinet.h"

#include <cassert>

#include "milinet/worker.h"

namespace milinet {

Milinet::Milinet(std::string_view config_path)
    : service_mgr_(this)
    , msg_mgr_(this) {
    auto work_thread_num = kWorkThreadNum;
    workers_.reserve(work_thread_num);
    for (size_t i = 0; i < work_thread_num; ++i) {
        workers_.emplace_back(std::make_unique<Worker>(this));
    }
}

Milinet::~Milinet() = default;

void Milinet::Start() {
    for (auto& worker : workers_) {
        worker->Start();
    }
}

} //namespace milinet