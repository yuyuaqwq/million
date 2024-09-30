#include <iostream>

#include "milinet/milinet.h"
#include "milinet/service.h"

class TestService : public milinet::Service {
    using Base = milinet::Service;
    using Base::Base;

    virtual milinet::Task OnMsg(milinet::MsgUnique msg) override {
        std::cout << msg->session_id() << std::endl;

        auto res = co_await Recv(2);
        std::cout << res->session_id() << std::endl;

        res = co_await Recv(3);
        std::cout << res->session_id() << std::endl;

        co_await On4();
        co_await On5();
        co_return;
    }

    milinet::Task On4() {
        auto res = co_await Recv(4);
        std::cout << res->session_id() << std::endl;
        co_return;
    }

    milinet::Task On5() {
        auto res = co_await Recv(5);
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
  
    msg = std::make_unique<milinet::Msg>(net.AllocSessionId());
    net.Send(&service_ref, std::move(msg));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    msg = std::make_unique<milinet::Msg>(net.AllocSessionId());
    net.Send(&service_ref, std::move(msg));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    msg = std::make_unique<milinet::Msg>(net.AllocSessionId());
    net.Send(&service_ref, std::move(msg));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    msg = std::make_unique<milinet::Msg>(net.AllocSessionId());
    net.Send(&service_ref, std::move(msg));

    std::this_thread::sleep_for(std::chrono::seconds(100));

    return 0;
}
