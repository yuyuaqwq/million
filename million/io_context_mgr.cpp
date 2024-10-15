#include "million/io_context_mgr.h"

#include "million/io_context.h"

namespace million {

IoContextMgr::IoContextMgr(Million* million, size_t io_context_num)
    : million_(million) {
    if (io_context_num == 0) {
        io_context_num = std::thread::hardware_concurrency();
    }
    io_contexts_.reserve(io_context_num);
    for (size_t i = 0; i < io_context_num; ++i) {
        io_contexts_.emplace_back(std::make_unique<IoContext>(million_));
    }
}

IoContextMgr::~IoContextMgr() = default;

void IoContextMgr::Start() {
    for (auto& io_context : io_contexts_) {
        io_context->Start();
    }
}

void IoContextMgr::Stop() {
    for (auto& io_context : io_contexts_) {
        io_context->Stop();
    }
}

IoContext& IoContextMgr::NextIoContext() {
    auto index = ++index_;
    if (index == 0) {
        throw std::runtime_error("io index rolled back.");
    }
    return *io_contexts_[index % io_contexts_.size()];
}

} // namespace million