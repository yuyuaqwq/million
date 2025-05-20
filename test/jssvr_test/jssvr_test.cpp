#include <iostream>

#include <million/imillion.h>
#include <jssvr/jssvr.h>

#include <yaml-cpp/yaml.h>

#include <ss/ss_test.pb.h>

MILLION_MODULE_INIT();

class TestApp : public million::IMillion {
};

namespace ss = million::ss;

class TestService : public million::IService {
    MILLION_SERVICE_DEFINE(TestService);

public:
    using Base = million::IService;
    TestService(million::IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit() override {
        imillion().SetServiceName(service_handle(), "TestService");

        imillion().proto_mgr().codec().RegisterFile("ss/ss_test.proto", ss::msg_id, ss::test::sub_msg_id);

        return true;
    }

    MILLION_MESSAGE_HANDLE(ss::test::LoginReq, req) {
        logger().Info("ss::test::LoginReq, value:{}", req->value());

        // 回一个LoginRes
        co_return million::make_proto_message<ss::test::LoginRes>("LoginRes res");
    }

    MILLION_MESSAGE_HANDLE(ss::test::LoginRes, res) {
        logger().Info("ss::test::LoginRes, value:{}", res->value());
        co_return nullptr;
    }

};

int main() {
    auto test_app = std::make_unique<TestApp>();
    if (!test_app->Init("jssvr_test_settings.yaml")) {
        return 0;
    }
    auto svr = test_app->NewService<TestService>();
    
    test_app->Start();

    auto js_svr = million::jssvr::NewJSService(test_app.get(), "test");
    test_app->Send<ss::test::LoginReq>(*svr, *js_svr, "test_value");

    // test_app->Send(*service, *service, );

    std::this_thread::sleep_for(std::chrono::seconds(10000));


    return 0;
}
