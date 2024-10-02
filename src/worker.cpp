#include "milinet/worker.h"

#include "milinet/service.h"
#include "milinet/milinet.h"

namespace milinet {

Worker::Worker(Milinet* milinet) : milinet_(milinet) {

}

Worker::~Worker() = default;

void Worker::Start() {
    thread_ = std::make_unique<std::thread>([this]() {
        auto& service_mgr = milinet_->service_mgr();
        while (true) {
            auto& service = service_mgr.PopService();
            service.ProcessMsg();
            if (!service.MsgQueueEmpty()) {
                service_mgr.PushService(&service);
            }
        }
    });
}

void Worker::Join() {
    thread_->join();
}

void Worker::Detach() {
    thread_->detach();
}

} // namespace milinet