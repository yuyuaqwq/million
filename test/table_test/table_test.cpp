#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <table/table.pb.h>

MILLION_MODULE_INIT();

namespace protobuf = google::protobuf;
namespace table = million::table;

MILLION_MSG_DEFINE_EMPTY(, Test1Msg)

class TestService : public million::IService {
public:
    using Base = million::IService;
    TestService(million::IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit(million::MsgPtr msg) override {
        auto handle = imillion().GetServiceByName("DescService");
        if (!handle) {
            logger().Err("Unable to find DescService.");
            return false;
        }
        desc_service_ = *handle;

        return true;
    }

    virtual million::Task<million::MsgPtr> OnStart(::million::ServiceHandle sender, ::million::SessionId session_id) override {

        co_return nullptr;
    }


    MILLION_MSG_DISPATCH(TestService);

    MILLION_MSG_HANDLE(Test1Msg, msg) {

        co_return nullptr;
    }

private:
    million::ServiceHandle desc_service_;

};

class TestApp : public million::IMillion {
    //using Base = million::IMillion;
    //using Base::Base;
};

int main() {
    auto test_app = std::make_unique<TestApp>();
    if (!test_app->Init("table_test_config.yaml")) {
        return 0;
    }
    test_app->Start();

    auto service_opt = test_app->NewService<TestService>();
    if (!service_opt) {
        return 0;
    }
    auto service_handle = *service_opt;

    //test_app->Send<Test1Msg>(service_handle, service_handle);
    //for (int i = 0; i < 10000; i++) {
    //    auto msg2 = std::make_unique<Test1Msg>();
    //    test_app->Timeout(1, service_handle, std::move(msg2));
    //}

    std::this_thread::sleep_for(std::chrono::seconds(10000));

    return 0;
}
