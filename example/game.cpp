#include <iostream>

#include <million/imillion.h>
#include <million/iservice.h>


enum Test {
    kTest1,
    kTest2,
    kTest3,
};
using TestMsgBase = million::MsgBaseT<Test>;
MILLION_MSG_DEFINE(Test1Msg, kTest1, (int) value1, (std::string) value2);
MILLION_MSG_DEFINE(Test2Msg, kTest2, (int) value1, (std::string) value2);

class TestService : public million::IService {
    using Base = million::IService;
    using Base::Base;

    virtual million::Task OnMsg(million::MsgUnique msg) override {
        MILLION_MSG_DISPATCH(TestMsgBase, std::move(msg));

        auto res = co_await Recv<million::IMsg>(2);
        std::cout << res->session_id() << std::endl;

        res = co_await Recv<million::IMsg>(3);
        std::cout << res->session_id() << std::endl;

        co_await On4();
        co_await On5();
        co_return;
    }

    MILLION_MSG_HANDLE_INIT(TestMsgBase);

    MILLION_MSG_HANDLE_BEGIN(Test1Msg, msg) {
        std::cout << "Test1Msg" << msg->value1 << msg->value2 << std::endl;
        co_return;
    }
    MILLION_MSG_HANDLE_END(Test1Msg);


    MILLION_MSG_HANDLE_BEGIN(Test2Msg, msg) {
        std::cout << "Test2Msg" << msg->value1 << msg->value2 << std::endl;
        co_return;
    }
    MILLION_MSG_HANDLE_END(Test2Msg);


    million::Task On4() {
        auto res = co_await Recv<million::IMsg>(4);
        std::cout << res->session_id() << std::endl;
        co_return;
    }

    million::Task On5() {
        auto session_id = Send<Test1Msg>(service_handle(), 5, "hjh");
        auto res = co_await Recv<million::IMsg>(session_id);
        std::cout << res->session_id() << std::endl;

        res = co_await Call<million::IMsg, Test1Msg>(service_handle(), 6, "sbsb");

        co_return;
    }
};


int main() {
    auto mili = million::NewMillion("game_config.yaml");

    auto service_handle = mili->NewService<TestService>();

    mili->Send<Test1Msg>(service_handle, service_handle, 666, "sb");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    mili->Send<Test1Msg>(service_handle, service_handle, 2, "6");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    mili->Send<Test1Msg>(service_handle, service_handle, 3, "emm");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    mili->Send<Test1Msg>(service_handle, service_handle, 4, "hhh");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::this_thread::sleep_for(std::chrono::seconds(1000));

    return 0;
}
