#include <iostream>

#include <million/imillion.h>

#include <cluster/cluster_msg.h>

#include <protogen/ss/ss_test.pb.h>

namespace Ss = Million::Proto::Ss;
namespace protobuf = google::protobuf;

MILLION_MSG_DEFINE(, TestMsg, (Ss::Test::TestReq)req);

class TestService : public million::IService {
public:
    using Base = million::IService;
    TestService(million::IMillion* imillion)
        : proto_codec_(0)
        , Base(imillion) {}

    virtual bool OnInit() override {
        auto handle = imillion_->GetServiceByUniqueNum("ClusterService");
        if (!handle) {
            return false;
        }
        cluster_ = *handle;

        const protobuf::DescriptorPool* pool = protobuf::DescriptorPool::generated_pool();
        protobuf::DescriptorDatabase* db = pool->internal_generated_database();
        auto ss_test = pool->FindFileByName("ss/ss_test.proto");
        if (ss_test) {
            proto_codec_.RegisterProto(*ss_test, Ss::ss_msg_id, Ss::Test::ss_sub_msg_id_test);
        }
        return true;
    }

    MILLION_MSG_DISPATCH(TestService);

    MILLION_MSG_HANDLE(TestMsg, msg) {
        Send<million::cluster::ClusterSendPacketMsg>(cluster_, "TestService", "node2", "TestService", proto_codec_.EncodeMessage(msg->req).value());
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
    if (!test_app->Init("cluster_test_config.yaml")) {
        return 0;
    }

    auto service_opt = test_app->NewService<TestService>();
    if (!service_opt) {
        return 0;
    }
    auto service_handle = *service_opt;

    Ss::Test::TestReq req;
    req.set_value("sb");
    test_app->Send<TestMsg>(service_handle, service_handle, std::move(req));

    return 0;
}
