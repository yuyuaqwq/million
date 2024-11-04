#pragma once

#include <million/detail/dl_export.h>

namespace million {

class MILLION_CLASS_EXPORT noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

} // namespace million