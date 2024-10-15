#pragma once

#include <memory>
#include <vector>

#include "million/detail/noncopyable.h"

namespace million {

class Million;
class IoContext;
class IoContextMgr : noncopyable {
public:
    IoContextMgr(Million* million, size_t io_context_num);
    ~IoContextMgr();

    void Start();
    void Stop();

    IoContext& NextIoContext();

private:
    Million* million_;
    std::atomic_uint64_t index_;
    std::vector<std::unique_ptr<IoContext>> io_contexts_;
};

} // namespace million