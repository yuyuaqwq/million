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
        auto msg_ = static_cast<Test1Msg*>(msg.get());
        std::cout << msg_->session_id() << std::endl;
        std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        auto res = co_await Recv<million::IMsg>(2);
        msg_ = static_cast<Test1Msg*>(res.get());
        std::cout << msg_->session_id() << std::endl;
        std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        res = co_await Recv<million::IMsg>(3);
        msg_ = static_cast<Test1Msg*>(res.get());
        std::cout << msg_->session_id() << std::endl;
        std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        co_await On4();
        co_await On5();
        co_return;
    }



    million::Task On4() {
        auto res = co_await Recv<million::IMsg>(4);
        auto msg_ = static_cast<Test1Msg*>(res.get());
        std::cout << res->session_id() << std::endl;
        std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        co_return;
    }

    million::Task On5() {
        auto session_id = Send<Test1Msg>(service_handle(), 5, "hjh");
        auto res = co_await Recv<Test1Msg>(session_id);
        auto msg_ = static_cast<Test1Msg*>(res.get());
        std::cout << msg_->session_id() << std::endl;
        std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        res = co_await Call<Test1Msg, Test1Msg>(service_handle(), 6, "sbsb");
        msg_ = static_cast<Test1Msg*>(res.get());
        std::cout << msg_->session_id() << std::endl;
        std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        co_return;
    }
};

int main() {
    auto mili = million::NewMillion("game_config.yaml");

    auto start = std::chrono::high_resolution_clock::now();
    int j = 0;
    for (int i = 0; i < 1000000; i++) {
        mili->AddDelayTask({ 1, [&j](const auto& task) {
            ++j;
            // std::cout << "666" << std::endl;
            } });
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto duratioin = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Elapsed time: " << duratioin.count() << "ms:" << j << " seconds\n";

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
