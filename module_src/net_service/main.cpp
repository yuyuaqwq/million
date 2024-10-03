#include "iostream"

#include "milinet/imilinet.h"
#include "milinet/msg.h"

class TestMsg : public milinet::Msg {
public:
    TestMsg(int value1, std::string_view value2)
        : value1(value1)
        , value2(value2) {}

public:
    int value1;
    std::string value2;
};

class TestService : public milinet::IService {
    using Base = milinet::IService;
    using Base::Base;

    virtual milinet::Task OnMsg(milinet::MsgUnique msg) override {
        auto test_msg = static_cast<TestMsg*>(msg.get());
        std::cout << test_msg->session_id() << test_msg->value1 << test_msg->value2 << std::endl;

        auto res = co_await Recv<milinet::Msg>(2);
        std::cout << res->session_id() << std::endl;

        res = co_await Recv<milinet::Msg>(3);
        std::cout << res->session_id() << std::endl;

        co_await On4();
        co_await On5();
        co_return;
    }

    milinet::Task On4() {
        auto res = co_await Recv<milinet::Msg>(4);
        std::cout << res->session_id() << std::endl;
        co_return;
    }

    milinet::Task On5() {
        auto session_id = Send<TestMsg>(service_id(), 5, std::string_view("hjh"));
        auto res = co_await Recv<milinet::Msg>(session_id);
        std::cout << res->session_id() << std::endl;
        co_return;
    }
};

MILINET_FUNC_EXPORT bool MiliModuleInit(milinet::IMilinet* imilinet) {
    imilinet->CreateService<TestService>(imilinet);
    return true;
}
