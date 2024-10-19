#include <iostream>

#include <million/imillion.h>
#include <million/iservice.h>


enum Test {
    kTest1,
    kTest2,
    kTest3,
};

using TestMsgBase = million::MsgBaseT<Test>;

struct Test1Msg : million::MsgT<kTest1> {
    Test1Msg(auto&& value1, auto&& value2)
        : value1(value1), value2(value2) { }

    int value1;
    std::string value2;
};

struct Test2Msg : million::MsgT<kTest2> {
    Test2Msg(auto&& value1, auto&& value2)
        : value1(value1), value2(value2) {}

    int value1;
    std::string value2;
};

class TestService : public million::IService {
    using Base = million::IService;
    using Base::Base;

    virtual million::Task OnMsg(million::MsgUnique msg) override {

        MILLION_HANDLE_MSG_BEGIN(msg, TestMsgBase);

        MILLION_HANDLE_MSG(msg, Test1Msg, {
            std::cout << msg->session_id() << msg->value1 << msg->value2 << std::endl;
        });
        MILLION_HANDLE_MSG(msg, Test2Msg, {
            std::cout << msg->session_id() << msg->value1 << msg->value2 << std::endl;
        });

        MILLION_HANDLE_MSG_END();

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
        auto session_id = Send<Test1Msg>(service_handle(), 5, std::string_view("hjh"));
        auto res = co_await Recv<million::IMsg>(session_id);
        std::cout << res->session_id() << std::endl;

        res = co_await Call<million::IMsg, Test1Msg>(service_handle(), 6, std::string_view("sbsb"));

        co_return;
    }
};


int main() {
    auto mili = million::NewMillion("game_config.yaml");

    auto service_handle = mili->NewService<TestService>();

    mili->Send<Test1Msg>(service_handle, 666, std::string_view("sb"));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    mili->Send<Test1Msg>(service_handle, 2, "6");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    mili->Send<Test1Msg>(service_handle, 3, "emm");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    mili->Send<Test1Msg>(service_handle, 4, "hhh");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::this_thread::sleep_for(std::chrono::seconds(1000));

    return 0;
}
