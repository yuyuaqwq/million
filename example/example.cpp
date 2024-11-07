#include <iostream>

#include <million/imillion.h>
#include <million/iservice.h>



MILLION_MSG_DEFINE(, Test1Msg, (int) value1, (std::string) value2);
MILLION_MSG_DEFINE(, Test2Msg, (int) value1, (std::string) value2);

class TestService : public million::IService {
    using Base = million::IService;
    using Base::Base;

    virtual void OnInit() override {
        auto start = std::chrono::high_resolution_clock::now();
        int j = 0;
        //for (int i = 0; i < 100; i++) {
        //    Timeout<Test1Msg>(i * 100, 1, "emmm");
        //}
        auto end = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto duratioin = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Elapsed time: " << duratioin.count() << "ms:" << j << " seconds\n";
    }

    virtual million::Task OnMsg(million::MsgUnique msg) override {
        if (msg->type() == Test2Msg::kType) {
            auto msg_ = static_cast<Test2Msg*> (msg.get());
            std::cout << msg_->session_id() << std::endl;
            std::cout << "Test2Msg" << msg_->value1 << msg_->value2 << std::endl;
            co_return;
        }

        auto msg_ = static_cast<Test1Msg*> (msg.get());
        std::cout << msg_->session_id() << std::endl;
        std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        auto res = co_await Recv<million::IMsg>(4);
        msg_ = static_cast<Test1Msg*>(res.get());
        std::cout << msg_->session_id() << std::endl;
        std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        res = co_await Recv<million::IMsg>(5);
        msg_ = static_cast<Test1Msg*>(res.get());
        std::cout << msg_->session_id() << std::endl;
        std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        auto task = On4();
        co_await task;
        task.rethrow_if_exception();

        co_await On5();

        std::cout << "end" << std::endl;

        co_return;
    }

    million::Task On4() {
        throw std::runtime_error("sb");
        auto res = co_await Recv<million::IMsg>(6);
        throw std::runtime_error("sb2");
        auto msg_ = static_cast<Test1Msg*>(res.get());
        std::cout << res->session_id() << std::endl;
        std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        co_return;
    }

    million::Task On5() {
        auto session_id = Send<Test1Msg>(service_handle(), 7, "hjh");
        auto res = co_await Recv<Test1Msg>(session_id);
        auto msg_ = static_cast<Test1Msg*>(res.get());
        std::cout << msg_->session_id() << std::endl;
        std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        res = co_await Call<Test1Msg, Test1Msg>(service_handle(), 8, "sbsb");
        msg_ = static_cast<Test1Msg*>(res.get());
        std::cout << msg_->session_id() << std::endl;
        std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        co_return;
    }
};

int main() {
    auto mili = million::NewMillion("example_config.yaml");


    auto service_handle = mili->NewService<TestService>();

    mili->Send<Test1Msg>(service_handle, service_handle, 3, "sb");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    mili->Send<Test1Msg>(service_handle, service_handle, 4, "6");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    mili->Send<Test1Msg>(service_handle, service_handle, 5, "emm");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    mili->Send<Test1Msg>(service_handle, service_handle, 6, "hhh");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::this_thread::sleep_for(std::chrono::seconds(1000));

    return 0;
}
