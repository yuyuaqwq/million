#pragma once

#include <functional>

#include <million/detail/dl_export.h>

namespace million {
namespace detail {

struct DelayTask {
    using Func = std::function<void(const DelayTask&)>;

    DelayTask(uint32_t tick, const Func& func)
        : tick(tick)
        , func(func) {}
    ~DelayTask() = default;

    DelayTask(const DelayTask&) = default;
    DelayTask(DelayTask&&) = default;

    uint32_t tick;
    Func func;
};

} // namespace detail
} // namespace million