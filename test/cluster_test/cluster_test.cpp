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

using ClusterRecvPacketMsg = million::cluster::ClusterRecvPacketMsg;

class TestService : public million::IService {
public:
    using Base = million::IService;
    TestService(million::IMillion* imillion)
        : Base(imillion)
        , proto_codec_(million::GetDescriptorPool(), million::GetDescriptorDatabase(), million::GetMessageFactory()) {}

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

        proto_codec_.RegisterProto("ss/ss_test.proto", ss::msg_id, ss::test::sub_msg_id);

        return true;
    }

    MILLION_MSG_DISPATCH(TestService);

    MILLION_PROTO_PACKET_DISPATCH(&proto_codec_, ClusterRecvPacketMsg);

    using LoginReq = ss::test::LoginReq;
    MILLION_PROTO_MSG_HANDLE(LoginReq, req) {
        logger().Info("test, value:{}", req->value());
        co_return;
    }

    MILLION_CPP_MSG_HANDLE(TestMsg, msg) {
        Send<million::cluster::ClusterSendPacketMsg>(cluster_, "TestService", msg->target_node, "TestService", proto_codec_.EncodeMessage(msg->req).value());
        co_return;
    }

private:
    million::ServiceHandle cluster_;

    million::ProtoCodec proto_codec_;
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
    req.set_value("sb");

    const auto& config = test_app->YamlConfig();
    if (config["cluster"]["name"].as<std::string>() == "node1") {
        test_app->Send<TestMsg>(service_handle, service_handle, "node2", std::move(req));
    }
    else {
        test_app->Send<TestMsg>(service_handle, service_handle, "node1", std::move(req));
    }
   
    return 0;
}
