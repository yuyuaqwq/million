#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <ss/ss_test.pb.h>

MILLION_MODULE_INIT();

class TestApp : public million::IMillion {
};

namespace ss = million::ss;

class TestService : public million::IService {
public:
    using Base = million::IService;
    TestService(million::IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit() override {
        imillion().SetServiceName(service_handle(), "TestService");

        imillion().proto_mgr().codec().RegisterFile("ss/ss_test.proto", ss::msg_id, ss::test::sub_msg_id);

        return true;
    }

    MILLION_MSG_DISPATCH(TestService);

    using LoginReq = ss::test::LoginReq;
    MILLION_MSG_HANDLE(LoginReq, req) {
        logger().Info("ss::test::LoginReq, value:{}", req->value());

        // 回一个LoginRes
        co_return million::make_proto_msg<ss::test::LoginRes>("LoginRes res");
    }

    using LoginRes = ss::test::LoginRes;
    MILLION_MSG_HANDLE(LoginRes, res) {
        logger().Info("ss::test::LoginRes, value:{}", res->value());
        co_return nullptr;
    }

};

int main() {
    auto test_app = std::make_unique<TestApp>();
    if (!test_app->Init("jssvr_test_settings.yaml")) {
        return 0;
    }
    test_app->NewService<TestService>();
    
    test_app->Start();

    std::this_thread::sleep_for(std::chrono::seconds(10000));


    return 0;
}
