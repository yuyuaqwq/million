#include <iostream>

#include <million/imillion.h>
#include <million/exception.h>

MILLION_MODULE_INIT();


class TestService : public million::IService {
    using Base = million::IService;
    using Base::Base;

    virtual bool OnInit() override {
        //auto start = std::chrono::high_resolution_clock::now();
        //int j = 0;
        ////for (int i = 0; i < 100; i++) {
        ////    Timeout<Test1Msg>(i * 100, 1, "emmm");
        ////}
        //auto end = std::chrono::high_resolution_clock::now();
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        //auto duratioin = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        //std::cout << "Elapsed time: " << duratioin.count() << "ms:" << j << " seconds\n";

        return true;
    }

    virtual million::Task<> OnMsg(million::SessionId session_id, million::MsgUnique msg) override {
        //if (msg->type() == Test2Msg::type_static()) {
        //    auto msg_ = static_cast<Test2Msg*> (msg.get());
        //    std::cout << msg_->session_id() << std::endl;
        //    std::cout << "Test2Msg" << msg_->value1 << msg_->value2 << std::endl;
        //    co_return;
        //}

        //// auto res1000 = co_await RecvWithTimeout<million::IMsg>(1000, 5);

        //auto msg_ = static_cast<Test1Msg*> (msg.get());
        //std::cout << msg_->session_id() << std::endl;
        //std::cout << "1.Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        //auto res = co_await Recv<million::IMsg>(12);
        //msg_ = static_cast<Test1Msg*>(res.get());
        //std::cout << msg_->session_id() << std::endl;
        //std::cout << "2.Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        //res = co_await Recv<million::IMsg>(13);
        //msg_ = static_cast<Test1Msg*>(res.get());
        //std::cout << msg_->session_id() << std::endl;
        //std::cout << "3.Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

        //co_await On6();
        //auto res2 = co_await On8();

        //std::cout << "end:" << res2->session_id() << std::endl;

        co_return;
    }

    //million::Task<> On6() {
    //    //throw std::runtime_error("sb");
    //    auto res = co_await Recv<million::IMsg>(14);
    //    //throw std::runtime_error("sb2");
    //    auto msg_ = static_cast<Test1Msg*>(res.get());
    //    std::cout << res->session_id() << std::endl;
    //    std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;
    //    auto i = co_await On7();

    //    co_return;
    //}

    //million::Task<int> On7() {
    //    auto res = co_await Recv<million::IMsg>(15);
    //    //throw std::runtime_error("sb2");
    //    auto msg_ = static_cast<Test1Msg*>(res.get());
    //    std::cout << res->session_id() << std::endl;
    //    std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;
    //    co_return 1;
    //}

    //million::Task<std::unique_ptr<Test1Msg>> On8() {
    //    auto session_id = Send<Test1Msg>(service_handle(), 8, "hjh");
    //    auto res = co_await Recv<Test1Msg>(session_id);
    //    auto msg_ = static_cast<Test1Msg*>(res.get());
    //    std::cout << msg_->session_id() << std::endl;
    //    std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

    //    res = co_await Call<Test1Msg, Test1Msg>(service_handle(), 9, "sbsb");
    //    msg_ = static_cast<Test1Msg*>(res.get());
    //    std::cout << msg_->session_id() << std::endl;
    //    std::cout << "Test1Msg" << msg_->value1 << msg_->value2 << std::endl;

    //    co_return std::move(res);
    //}
};

class TestApp : public million::IMillion {
    //using Base = million::IMillion;
    //using Base::Base;
};


int main() {
    // auto a = million::TaskAbortException("sb{}", 21);

    // auto str = std::format("{}", a.stacktrace());

    auto test_app = std::make_unique<TestApp>();
    if (!test_app->Start("task_test_config.yaml")) {
        return 0;
    }

    // std::is_enum_v<million::ss::logger::LogLevel&>;

    test_app->logger().Info("sbsb");

    auto service_opt = test_app->NewService<TestService>();
    if (!service_opt) {
        return 0;
    }
    auto service_handle = *service_opt;

    //test_app->Send<Test1Msg>(service_handle, service_handle, 11, "sb");

    //std::this_thread::sleep_for(std::chrono::seconds(1));

    //test_app->Send<Test1Msg>(service_handle, service_handle, 12, "6");

    //std::this_thread::sleep_for(std::chrono::seconds(1));
    //test_app->Send<Test1Msg>(service_handle, service_handle, 13, "emm");

    //std::this_thread::sleep_for(std::chrono::seconds(1));
    //test_app->Send<Test1Msg>(service_handle, service_handle, 14, "hhh");

    //std::this_thread::sleep_for(std::chrono::seconds(1));
    //test_app->Send<Test2Msg>(service_handle, service_handle, 15, "hhh");

    std::this_thread::sleep_for(std::chrono::seconds(1000));

    return 0;
}
