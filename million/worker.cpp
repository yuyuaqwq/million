#include "million/worker.h"

#include "million/million.h"
#include "million/service_mgr.h"
#include "million/service.h"

namespace million {

Worker::Worker(Million* million) : million_(million) {

}

Worker::~Worker() = default;

void Worker::Start() {
    thread_ = std::make_unique<std::jthread>([this]() {
        auto& service_mgr = million_->service_mgr();
        while (true) {
            auto& service = service_mgr.PopService();
            service.ProcessMsg();
            if (!service.MsgQueueEmpty()) {
                service_mgr.PushService(&service);
            }
        }
    });
}

} // namespace million