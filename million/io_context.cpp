#include "million/io_context.h"

#include "million/million.h"
#include "million/service_mgr.h"
#include "million/service.h"
#include <iostream>

namespace million {

IoContext::IoContext(Million* million)
    : million_(million) {}

IoContext::~IoContext() = default;

void IoContext::Start() {
    work_.emplace(io_context_);
    thread_.emplace([this]() {
        io_context_.run();
    });
}

void IoContext::Stop() {
    work_ = std::nullopt;
    thread_->join();
}

} // namespace million