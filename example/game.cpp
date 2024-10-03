#include <iostream>

#include "milinet/milinet.h"
#include "milinet/service.h"

#include "milinet/detail/time_wheel.hpp"
#include "milinet/detail/time_wheel_heap.hpp"

//class TestMsg : public milinet::Msg {
//public:
//    TestMsg(int value1, std::string_view value2)
//        : value1(value1)
//        , value2(value2) {}
//
//public:
//    int value1;
//    std::string value2;
//};
//
//class TestService : public milinet::IService {
//    using Base = milinet::IService;
//    using Base::Base;
//
//    virtual milinet::Task OnMsg(milinet::MsgUnique msg) override {
//        auto test_msg = static_cast<TestMsg*>(msg.get());
//        std::cout << test_msg->session_id() << test_msg->value1 << test_msg->value2 << std::endl;
//
//        auto res = co_await Recv<milinet::Msg>(2);
//        std::cout << res->session_id() << std::endl;
//
//        res = co_await Recv<milinet::Msg>(3);
//        std::cout << res->session_id() << std::endl;
//
//        co_await On4();
//        co_await On5();
//        co_return;
//    }
//
//    milinet::Task On4() {
//        auto res = co_await Recv<milinet::Msg>(4);
//        std::cout << res->session_id() << std::endl;
//        co_return;
//    }
//
//    milinet::Task On5() {
//        auto session_id = Send<TestMsg>(service_id(), 5, std::string_view("hjh"));
//        auto res = co_await Recv<milinet::Msg>(session_id);
//        std::cout << res->session_id() << std::endl;
//        co_return;
//    }
//};


int main() {

    int count = 0;
    auto tw = milinet::detail::MinHeapTimer();
    // tw.Init();

    auto start = std::chrono::high_resolution_clock::now(); // 开始时间


    int j = 0;
    for (int i = 0; i < 1000000; i++) {
        tw.AddTask(1, [&j](const milinet::detail::MinHeapTimer::Task& task) {
            ++j;
            // std::cout << "666" << std::endl;
        });
    }

    tw.Tick();

    auto end = std::chrono::high_resolution_clock::now(); // 结束时间
    auto duratioin = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Elapsed time: " << duratioin.count() << "ms:" << j << " seconds\n";

    milinet::Milinet net("game_config.yaml");
    net.Init();
    net.Start();

    // auto service_id = net.CreateService<TestService>();
    // 
    // service_mgr.Send<TestMsg>(service.service_id(), 666, std::string_view("sb"));

    //net.Send<TestMsg>(service_id, 666, std::string_view("sb"));

    //// service_mgr.Send(&service, net.MakeMsg<TestMsg>(666, std::string_view("sb")));

    //std::this_thread::sleep_for(std::chrono::seconds(1));
    //net.Send<TestMsg>(service_id, 2, "6");

    //std::this_thread::sleep_for(std::chrono::seconds(1));
    //net.Send<TestMsg>(service_id, 3, "emm");

    //std::this_thread::sleep_for(std::chrono::seconds(1));
    //net.Send<TestMsg>(service_id, 4, "hhh");

    // std::this_thread::sleep_for(std::chrono::seconds(1));

    std::this_thread::sleep_for(std::chrono::seconds(100));

    return 0;
}
