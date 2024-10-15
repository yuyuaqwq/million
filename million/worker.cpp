#include "million/worker.h"

#include "million/million.h"
#include "million/service_mgr.h"
#include "million/service.h"
#include <iostream>
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