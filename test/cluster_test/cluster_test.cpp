#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <cluster/cluster.h>

#include <ss/ss_test.pb.h>

namespace ss = million::ss;
namespace protobuf = google::protobuf;

MILLION_MESSAGE_DEFINE_NONCOPYABLE(, TestMsg, (million::cluster::NodeName) target_node, (million::ProtoMessageUnique) req);

class TestService : public million::IService {
    MILLION_SERVICE_DEFINE(TestService);

public:
    using Base = million::IService;
    TestService(million::IMillion* imillion)
        : Base(imillion) {}

    //TestService(million::IMillion* imillion)
    //    : proto_codec_(0)
    //    , Base(imillion) {}

    virtual bool OnInit() override {
        imillion().SetServiceName(service_handle(), "TestService");

        auto handle = imillion().GetServiceByName(million::cluster::kClusterServiceName);
        if (!handle) {
            return false;
        }
        cluster_ = *handle;

        imillion().proto_mgr().codec().RegisterFile("ss/ss_test.proto", ss::msg_id, ss::test::sub_msg_id);

        return true;
    }

    MILLION_MESSAGE_HANDLE(ss::test::LoginReq, req) {
        logger().Info("ss::test::LoginReq, value:{}", req->value());

        // 回复LoginRes
        co_return million::make_proto_message<ss::test::LoginRes>("LoginRes res");
    }

    MILLION_MESSAGE_HANDLE(ss::test::LoginRes, res) {
        logger().Info("ss::test::LoginRes, value:{}", res->value());
        co_return nullptr;
    }

    MILLION_MESSAGE_HANDLE(TestMsg, msg) {
        Send<million::cluster::ClusterSend>(cluster_
            , "TestService", msg->target_node, "TestService"
            , std::move(msg->req));
        co_return nullptr;
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
    if (!test_app->Init("cluster_test_settings.yaml")) {
        return 0;
    }
    test_app->Start();



    test_app->proto_mgr().codec().RegisterFile("ss/ss_test.proto", million::ss::msg_id, million::ss::test::sub_msg_id);

    auto service_opt = test_app->NewService<TestService>();
    if (!service_opt) {
        return 0;
    }
    auto service_handle = *service_opt;

    getchar();

    auto req = million::make_proto_message<ss::test::LoginReq>("LoginReq req");

    const auto& settings = test_app->YamlSettings();
    if (settings["cluster"]["name"].as<std::string>() == "node1") {
        test_app->Send<TestMsg>(service_handle, service_handle, "node2", std::move(req));
    }
    else {
        test_app->Send<TestMsg>(service_handle, service_handle, "node1", std::move(req));
    }
   
    return 0;
}
