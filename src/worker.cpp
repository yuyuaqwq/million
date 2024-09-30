#include "milinet/worker.h"

#include "milinet/service.h"
#include "milinet/milinet.h"

#include "asio.hpp"

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
}

void Worker::Join() {
    thread_->join();
}

void Worker::Detach() {
    thread_->detach();
}

} // namespace milinet