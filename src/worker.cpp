#include "milinet/worker.h"

#include <system_error>

#include "milinet/service.h"
#include "milinet/milinet.h"

namespace milinet {

Worker::Worker(Milinet* milinet) : milinet_(milinet) {

}

Worker::~Worker() = default;

void Worker::Start() {
    thread_ = std::make_unique<std::thread>([this]() {
        while (true) {
            auto& service = milinet_->PopService();
            service.ProcessMsg();
            if (!service.MsgQueueEmpty()) {
                milinet_->PushService(&service);
            }
        }
    });
    if (!thread_) {
        throw std::system_error(
            std::make_error_code(std::errc::resource_unavailable_try_again), 
            "failed to create thread."
        );
    }
}

void Worker::Join() {
    thread_->join();
}

void Worker::Detach() {
    thread_->detach();
}

} // namespace milinet