#include <iostream>

#include "milinet/milinet.h"
#include "milinet/service.h"

// #include "asio.hpp"

#include "milinet/dl.hpp"

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
        auto res = co_await Recv(5);
        std::cout << res->session_id() << std::endl;
        co_return;
    }
};


int main() {
    milinet::Dll dll;
    dll.Load("../lib/libnetservice.so");
    auto func = dll.GetFuncPtr<int()>("sb");
    auto ret = func();
    // auto ret = dll.Call<int>(func, 1);

    milinet::Milinet net(1);

    
    auto session_id = net.AllocServiceId();
    auto service = std::make_unique<TestService>(&net, session_id);
    auto& service_ref = net.AddService(std::move(service));

    net.Start();

    // net.Send<TestMsg>(service_ref.id(), 666, std::string_view("sb"));

    service_ref.Send<TestMsg>(service_ref.id(), 666, std::string_view("sb"));

    // net.Send(&service_ref, net.MakeMsg<TestMsg>(666, std::string_view("sb")));

    std::this_thread::sleep_for(std::chrono::seconds(1));
    net.Send(&service_ref, net.MakeMsg());

    std::this_thread::sleep_for(std::chrono::seconds(1));
    net.Send(&service_ref, net.MakeMsg());

    std::this_thread::sleep_for(std::chrono::seconds(1));
    net.Send(&service_ref, net.MakeMsg());

    std::this_thread::sleep_for(std::chrono::seconds(1));
    net.Send(&service_ref, net.MakeMsg());

    std::this_thread::sleep_for(std::chrono::seconds(100));

    return 0;
}
