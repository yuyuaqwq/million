#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <cluster/cluster.h>

#include <test/ss_test.pb.h>
#include <cluster/ss_cluster.pb.h>

namespace test = million::test;
namespace module = million::module;
namespace cluster = million::cluster;
namespace protobuf = google::protobuf;

MILLION_MESSAGE_DEFINE_NONCOPYABLE(, TestMsg, (million::ModuleCode) target_service_name_id, (million::ProtoMessageUnique) req);

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
        auto handle = imillion().FindServiceByNameId(module::module_id, cluster::ss::ServiceNameId_descriptor(), cluster::ss::SERVICE_NAME_ID_CLUSTER);
        if (!handle) {
            return false;
        }
        cluster_ = *handle;

        imillion().proto_mgr().codec().RegisterFile("test/ss_test.proto", module::module_id, test::ss::ss_msg_id);

        return true;
    }

    MILLION_MESSAGE_HANDLE(test::ss::LoginReq, req) {
        logger().LOG_INFO("test::ss::LoginReq, value:{}", req->value());

        // 回复LoginRes
        co_return million::make_proto_message<test::ss::LoginRes>("LoginRes res");
    }

    MILLION_MESSAGE_HANDLE(TestMsg, msg) {
        auto res = co_await Call<million::cluster::ClusterCallMessage, test::ss::LoginRes>(cluster_,
            msg->target_service_name_id,
            std::move(msg->req));
        logger().LOG_INFO("test::ss::LoginRes, value:{}", res->value());
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

    // test_app->proto_mgr().codec().RegisterFile("ss/ss_test.proto", module::module_id, test::ss::ss_msg_id);

    auto service_opt = test_app->NewService<TestService>();
    if (!service_opt) {
        return 0;
    }
    auto service_handle = *service_opt;

    const auto& settings = test_app->YamlSettings();
    if (settings["node"]["id"].as<int>() == 1) {
        test_app->SetServiceNameId(service_handle, module::module_id, test::ss::ServiceNameId_descriptor(), test::ss::SERVICE_NAME_ID_TEST_A);

    }
    else {
        test_app->SetServiceNameId(service_handle, module::module_id, test::ss::ServiceNameId_descriptor(), test::ss::SERVICE_NAME_ID_TEST_B);

    }

    getchar();

    auto req = million::make_proto_message<test::ss::LoginReq>("LoginReq req");

    if (settings["node"]["id"].as<int>() == 1) {
        test_app->Send<TestMsg>(service_handle, service_handle,
            million::EncodeModuleCode(module::module_id, test::ss::ServiceNameId_descriptor(), test::ss::SERVICE_NAME_ID_TEST_B),
            std::move(req));
    }
    else {
        test_app->Send<TestMsg>(service_handle, service_handle,
            million::EncodeModuleCode(module::module_id, test::ss::ServiceNameId_descriptor(), test::ss::SERVICE_NAME_ID_TEST_A),
            std::move(req));
    }
   
    return 0;
}
