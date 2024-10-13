#include <iostream>

#include "million/imillion.h"
#include "million/iservice.h"

class TestMsg : public million::IMsg {
public:
    TestMsg(int value1, std::string_view value2)
        : value1(value1)
        , value2(value2) {}

public:
    int value1;
    std::string value2;
};

class TestService : public million::IService {
    using Base = million::IService;
    using Base::Base;

    virtual million::Task OnMsg(million::MsgUnique msg) override {
        auto test_msg = static_cast<TestMsg*>(msg.get());
        std::cout << test_msg->session_id() << test_msg->value1 << test_msg->value2 << std::endl;

        auto res = co_await Recv<million::IMsg>(2);
        std::cout << res->session_id() << std::endl;

        res = co_await Recv<million::IMsg>(3);
        std::cout << res->session_id() << std::endl;

        co_await On4();
        co_await On5();
        co_return;
    }

    million::Task On4() {
        auto res = co_await Recv<million::IMsg>(4);
        std::cout << res->session_id() << std::endl;
        co_return;
    }

    million::Task On5() {
        auto session_id = Send<TestMsg>(service_handle(), 5, std::string_view("hjh"));
        auto res = co_await Recv<million::IMsg>(session_id);
        std::cout << res->session_id() << std::endl;
        co_return;
    }
};


int main() {
    auto mili = million::NewMillion("game_config.yaml");

    auto service_handle = mili->NewService<TestService>();

    mili->Send<TestMsg>(service_handle, 666, std::string_view("sb"));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    mili->Send<TestMsg>(service_handle, 2, "6");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    mili->Send<TestMsg>(service_handle, 3, "emm");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    mili->Send<TestMsg>(service_handle, 4, "hhh");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::this_thread::sleep_for(std::chrono::seconds(100));

    return 0;
}
