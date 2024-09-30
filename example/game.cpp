#include <iostream>

#include "milinet/milinet.h"
#include "milinet/service.h"

class TestService : public milinet::Service {
    using Base = milinet::Service;
    using Base::Base;

    virtual milinet::SessionCoroutine OnMsg(milinet::MsgUnique msg) override {
        auto res = co_await Recv(1);
        std::cout << res->session_id() << std::endl;

        res = co_await Recv(2);
        std::cout << res->session_id() << std::endl;
        co_return;
    }
};


int main() {
    milinet::Milinet net(1);

    auto service = std::make_unique<TestService>(&net, net.AllocServiceId());
    auto& service_ref = net.AddService(std::move(service));

    net.Start();

    auto msg = std::make_unique<milinet::Msg>(net.AllocSessionId());
    net.Send(&service_ref, std::move(msg));

    std::this_thread::sleep_for(std::chrono::seconds(1));
  
    msg = std::make_unique<milinet::Msg>(1);
    net.Send(&service_ref, std::move(msg));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    msg = std::make_unique<milinet::Msg>(2);
    net.Send(&service_ref, std::move(msg));

    std::this_thread::sleep_for(std::chrono::seconds(100));

    return 0;
}
