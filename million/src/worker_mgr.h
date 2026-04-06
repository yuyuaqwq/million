#pragma once

#include <memory>
#include <vector>

#include <million/noncopyable.h>

namespace million {

class Million;
class Worker;
class WorkerManager : noncopyable {
public:
    WorkerManager(Million* million, size_t worker_num);
    ~WorkerManager();

    void Start();
    void Stop();

private:
    Million* million_;
    std::vector<std::unique_ptr<Worker>> workers_;
};

} // namespace million