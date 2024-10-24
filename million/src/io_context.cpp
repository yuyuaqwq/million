#include "io_context.h"

#include <iostream>

#include "million.h"
#include "service_mgr.h"
#include "service.h"

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
}

} // namespace million