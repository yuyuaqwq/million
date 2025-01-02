#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <ss/ss_test.pb.h>

MILLION_MODULE_INIT();

class TestApp : public million::IMillion {
    //using Base = million::IMillion;
    //using Base::Base;
};

namespace ss = million::ss;

class TestService : public million::IService {
public:
    using Base = million::IService;
    TestService(million::IMillion* imillion)
        : Base(imillion) {}

    //TestService(million::IMillion* imillion)
    //    : proto_codec_(0)
    //    , Base(imillion) {}

    virtual bool OnInit(million::MsgUnique msg) override {
        imillion().SetServiceName(service_handle(), "TestService");

        imillion().proto_mgr().codec().RegisterFile("ss/ss_test.proto", ss::msg_id, ss::test::sub_msg_id);

        return true;
    }

    MILLION_MSG_DISPATCH(TestService);

    using LoginReq = ss::test::LoginReq;
    MILLION_MSG_HANDLE(LoginReq, req) {
        logger().Info("ss::test::LoginReq, value:{}", req->value());

        // »ØÒ»¸öLoginRes
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
    if (!test_app->Init("jssvr_test_config.yaml")) {
        return 0;
    }
    test_app->NewService<TestService>();
    
    test_app->Start();

    std::this_thread::sleep_for(std::chrono::seconds(10000));


    return 0;
}
