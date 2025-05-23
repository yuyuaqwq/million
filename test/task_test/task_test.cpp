#include <iostream>

#include <million/imillion.h>

#include <gateway/gateway.h>

namespace gateway = million::gateway;

MILLION_MODULE_INIT();

MILLION_MESSAGE_DEFINE(, Test2Msg, (int) value1, (std::string) value2);

class TestService : public million::IService {
    MILLION_SERVICE_DEFINE(TestService);

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

        /*auto handle = imillion().GetServiceByName(million::gateway::kGatewayServiceName);
        if (!handle) {
            logger().LOG_ERROR("GatewayService not found.");
            return false;
        }
        gateway_ = *handle;*/

        return true;
    }

    virtual million::Task<million::MessagePointer> OnStart(million::ServiceHandle sender, million::SessionId session_id, million::MessagePointer with_msg) override {
        /*auto res = co_await Call<gateway::GatewayRegisterUserServiceMsg>(gateway_, service_handle());
        logger().LOG_INFO("GatewayRegisterUserServiceMsg success.");*/
        co_return nullptr;
    }

    virtual million::Task<million::MessagePointer> OnMsg(million::ServiceHandle sender, million::SessionId session_id, million::MessagePointer msg) override {
        auto res1000 = co_await RecvWithTimeout<Test2Msg>(1000, 5);
        auto res = co_await On6();

        if (msg.IsType<Test2Msg>()) {
            auto msg_ = msg.GetMessage<Test2Msg>();
            std::cout << session_id << std::endl;
            std::cout << "Test2Msg" << msg_->value1 << msg_->value2 << std::endl;
            co_return nullptr;
        }

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

        co_return nullptr;
    }

    million::Task<million::MessagePointer> On6() {
        co_return nullptr;
    }

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

    million::ServiceHandle gateway_;
};

class TestApp : public million::IMillion {
    //using Base = million::IMillion;
    //using Base::Base;
};




int main() {
    // auto a = million::TaskAbortException("sb{}", 21);

    // auto str = std::format("{}", a.stacktrace());

    auto test_app = std::make_unique<TestApp>();
    if (!test_app->Init("task_test_settings.yaml")) {
        return 0;
    }

    test_app->Start();

    // std::is_enum_v<million::ss::logger::LogLevel&>;

    test_app->logger().LOG_INFO("sbsb");

    auto service_opt = test_app->NewService<TestService>();
    if (!service_opt) {
        return 0;
    }
    auto service_handle = *service_opt;

    test_app->SendTo<Test2Msg>(service_handle, service_handle, 666, 999, "sb");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    test_app->SendTo<Test2Msg>(service_handle, service_handle, million::SessionSendToReplyId(1000), 988, "sb");

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
