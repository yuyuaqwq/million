#include <iostream>

#include <yaml-cpp/yaml.h>

#include <million/imillion.h>
#include <million/jssvr/jssvr.h>
#include <million/test/ss_test.pb.h>

MILLION_MODULE_INIT();

namespace test = million::test;
namespace module = million::module;
namespace protobuf = google::protobuf;

class TestApp : public million::IMillion {
};

class TestService : public million::IService {
    MILLION_SERVICE_DEFINE(TestService);

public:
    using Base = million::IService;
    TestService(million::IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit() override {
        imillion().SetServiceNameId(service_handle(), module::module_id, test::ss::ServiceNameId_descriptor(), test::ss::SERVICE_NAME_ID_TEST_A);

        imillion().proto_mgr().codec().RegisterFile("test/ss_test.proto", module::module_id, test::ss::ss_msg_id);

        return true;
    }

    MILLION_MESSAGE_HANDLE(test::ss::LoginReq, req) {
        logger().LOG_INFO("test::ss::LoginReq, value:{}", req->value());

        // 回一个LoginRes
        co_return million::make_proto_message<test::ss::LoginRes>("LoginRes res");
    }

    MILLION_MESSAGE_HANDLE(test::ss::LoginRes, res) {
        logger().LOG_INFO("test::ss::LoginRes, value:{}", res->value());
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
    test_app->Send<test::ss::LoginReq>(*svr, *js_svr, "test_value");

    // test_app->Send(*service, *service, );

    std::this_thread::sleep_for(std::chrono::seconds(10000));


    return 0;
}
