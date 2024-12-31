#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <cluster/cluster.h>

#include <million/proto_codec.h>

#include <protogen/protogen.h>
#include <protogen/ss/ss_test.pb.h>

namespace ss = million::ss;
namespace protobuf = google::protobuf;

MILLION_MSG_DEFINE(, TestMsg, (million::cluster::NodeName) target_node, (ss::test::LoginReq) req);

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

        auto handle = imillion().GetServiceByName("ClusterService");
        if (!handle) {
            return false;
        }
        cluster_ = *handle;

        imillion().proto_mgr().codec().RegisterFile("ss/ss_test.proto", ss::msg_id, ss::test::sub_msg_id);

        return true;
    }

    MILLION_MSG_DISPATCH(TestService);

    using LoginReq = ss::test::LoginReq;
    MILLION_MSG_HANDLE(LoginReq, req) {
        logger().Info("ss::test::LoginReq, value:{}", req->value());

        // »ØÒ»¸öLoginRes
        ss::test::LoginRes res;
        res.set_value("LoginRes res");

        Reply<million::cluster::ClusterSendMsg>(sender, session_id, &res);

        co_return;
    }

    using LoginRes = ss::test::LoginRes;
    MILLION_MSG_HANDLE(LoginRes, res) {
        logger().Info("ss::test::LoginRes, value:{}", res->value());
        co_return;
    }

    MILLION_MSG_HANDLE(TestMsg, msg) {
        Send<million::cluster::ClusterSendWithNameMsg>(cluster_
            , "TestService", msg->target_node, "TestService"
            , &msg->req);
        co_return;
    }

private:
    million::ServiceHandle cluster_;
};

class TestApp : public million::IMillion {
    //using Base = million::IMillion;
    //using Base::Base;
};

int main() {
    auto test_app = std::make_unique<TestApp>();
    if (!test_app->Start("cluster_test_config.yaml")) {
        return 0;
    }

    auto service_opt = test_app->NewService<TestService>();
    if (!service_opt) {
        return 0;
    }
    auto service_handle = *service_opt;

    getchar();

    ss::test::LoginReq req;
    req.set_value("LoginRes req");

    const auto& config = test_app->YamlConfig();
    if (config["cluster"]["name"].as<std::string>() == "node1") {
        test_app->Send<TestMsg>(service_handle, service_handle, "node2", std::move(req));
    }
    else {
        test_app->Send<TestMsg>(service_handle, service_handle, "node1", std::move(req));
    }
   
    return 0;
}
