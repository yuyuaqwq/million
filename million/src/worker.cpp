#include "worker.h"

#include <iostream>

#include "million.h"
#include "service_mgr.h"
#include "service.h"

namespace million {

Worker::Worker(Million* million) : million_(million) {

}

Worker::~Worker() = default;

void Worker::Start() {
    thread_.emplace([this]() {
        auto& service_mgr = million_->service_mgr();
        while (true) {
            auto& service = service_mgr.PopService();
            // std::cout << "workid:" <<  std::this_thread::get_id() << std::endl;
            service.ProcessMsg();
            // 可以将service放到队列了
            service.set_in_queue(false);
            if (!service.MsgQueueEmpty()) {
                service_mgr.PushService(&service);
            }
        }
    });
}

} // namespace million