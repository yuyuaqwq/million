#include "milinet/worker.h"

#include "milinet/milinet.h"
#include "milinet/service_mgr.h"
#include "milinet/service.h"

namespace milinet {

Worker::Worker(Milinet* milinet) : milinet_(milinet) {

}

Worker::~Worker() = default;

void Worker::Start() {
    thread_ = std::make_unique<std::jthread>([this]() {
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

} // namespace milinet