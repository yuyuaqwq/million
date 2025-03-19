#include "worker.h"

#include <iostream>

#include "million.h"
#include "service_mgr.h"
#include "service_core.h"

namespace million {

Worker::Worker(Million* million)
    : million_(million) {}

Worker::~Worker() = default;

void Worker::Start() {
    thread_.emplace([this]() {
        run_ = true;
        auto& service_mgr = million_->service_mgr();
        while (run_) {
            auto service = service_mgr.PopService();
            if (!service) break;
            // std::cout << "workid:" <<  std::this_thread::get_id() << std::endl;
            service->ProcessMsgs(1);
            // 可以将service放到队列了
            service->set_in_queue(false);
            if (!service->MsgQueueIsEmpty()) {
                service_mgr.PushService(service);
                continue;
            }
            if (service->IsExited()) {
                // 服务已退出，销毁
                service_mgr.DeleteService(service);
            }
        }
    });
}

void Worker::Stop() {
    run_ = false;
    thread_.reset();
}

} // namespace million