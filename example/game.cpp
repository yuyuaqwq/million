#include <iostream>

#include "milinet/milinet.h"
#include "milinet/service.h"

// #include "asio.hpp"

#include "milinet/detail/dl.hpp"
#include "milinet/module.h"

class TestMsg : public milinet::Msg {
public:
    TestMsg(milinet::SessionId session_id, int value1, std::string_view value2)
        : milinet::Msg(session_id)
        , value1(value1)
        , value2(value2) {}

public:
    int value1;
    std::string value2;
};

class TestService : public milinet::Service {
    using Base = milinet::Service;
    using Base::Base;

    virtual milinet::Task OnMsg(milinet::MsgUnique msg) override {
        auto test_msg = static_cast<TestMsg*>(msg.get());
        std::cout << test_msg->session_id() << test_msg->value1 << test_msg->value2 << std::endl;

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
        auto session_id = Send<TestMsg>(service_id(), 5, std::string_view("好家伙"));
        auto res = co_await Recv(session_id);
        std::cout << res->session_id() << std::endl;
        co_return;
    }
};


int main() {
    milinet::Milinet net("game_config.yaml");

    auto& service = net.CreateService<TestService>();
    net.Start();

    // service_mgr.Send<TestMsg>(service.service_id(), 666, std::string_view("sb"));

    net.Send<TestMsg>(service.service_id(), 666, std::string_view("sb"));

    // service_mgr.Send(&service, net.MakeMsg<TestMsg>(666, std::string_view("sb")));

    std::this_thread::sleep_for(std::chrono::seconds(1));
    net.Send<TestMsg>(service.service_id(), 2, "6");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    net.Send<TestMsg>(service.service_id(), 3, "emm");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    net.Send<TestMsg>(service.service_id(), 4, "hhh");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::this_thread::sleep_for(std::chrono::seconds(100));

    return 0;
}
