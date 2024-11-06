#include "worker.h"

#include <iostream>

#include "million.h"
#include "service_mgr.h"
#include "service.h"

namespace million {

Worker::Worker(Million* million)
    : million_(million) {}

Worker::~Worker() = default;

void Worker::Start() {
    thread_.emplace([this]() {
        run_ = true;
        auto& service_mgr = million_->service_mgr();
        while (run_) {
            auto& service = service_mgr.PopService();
            // std::cout << "workid:" <<  std::this_thread::get_id() << std::endl;
            service.ProcessMsgs(1);
            // ���Խ�service�ŵ�������
            service.set_in_queue(false);
            if (service.MsgQueueEmpty()) {
                if (service.IsStoping()) {
                    // ֹͣ�����ٷ���
                    service.Close();
                }
                continue;
            }
            service_mgr.PushService(&service);
        }
    });
}

void Worker::Stop() {
    run_ = false;
}

} // namespace million