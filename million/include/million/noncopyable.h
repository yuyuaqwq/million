#pragma once

#include <million/api.h>

namespace million {

class MILLION_API noncopyable {
protected:
    noncopyable() = default;
    ~noncopyable() = default;

    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

    noncopyable(noncopyable&&) noexcept = default;
    noncopyable& operator=(noncopyable&&) noexcept = default;
};

} // namespace million