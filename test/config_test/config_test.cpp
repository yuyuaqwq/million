#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <million/config/config.h>
#include <million/config/ss_config.pb.h>

#include <million/test/cfg_network.pb.h>

MILLION_MODULE_INIT();

namespace protobuf = google::protobuf;
namespace config = million::config;
namespace module = million::module;
namespace test = million::test;

MILLION_MESSAGE_DEFINE_EMPTY(, Test1Msg)

class TestService : public million::IService {
    MILLION_SERVICE_DEFINE(TestService);

public:
    using Base = million::IService;
    TestService(million::IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit() override {
        auto handle = imillion().FindServiceByNameId(module::module_id, config::ss::ServiceNameId_descriptor(), config::ss::SERVICE_NAME_ID_CONFIG);
        if (!handle) {
            logger().LOG_ERROR("Unable to find ConfigService.");
            return false;
        }
        config_service_ = *handle;

        return true;
    }

    virtual million::Task<million::MessagePointer> OnStart(million::ServiceHandle sender, million::SessionId session_id, million::MessagePointer with_msg) override {
        tb_server_ = co_await config::QueryConfig<test::cfg::TbServer, test::cfg::Server>(this, config_service_);
        
        Send<Test1Msg>(service_handle());

        co_return nullptr;
    }


    MILLION_MESSAGE_HANDLE(Test1Msg, msg) {
        auto config_lock = co_await tb_server_.Lock(this, config_service_);

        million::ProtoFieldAny f;

        config_lock->BuildIndex(test::cfg::Server::kIdFieldNumber);
        config_lock->BuildIndex(test::cfg::Server::kTypeFieldNumber);

        auto aa = config_lock->FindRowByIndex(test::cfg::Server::kIdFieldNumber, 1001);
        auto bb = config_lock->FindRowByIndex(test::cfg::Server::kTypeFieldNumber, test::cfg::ServerType::ServerType_resource_download);

        config_lock->BuildCompositeIndex({ test::cfg::Server::kTypeFieldNumber, test::cfg::Server::kHostFieldNumber });
        auto aabb = config_lock->FindRowByCompositeIndex({ test::cfg::Server::kTypeFieldNumber, test::cfg::Server::kHostFieldNumber }
            , 1002, "127.0.0.1");

        auto row = config_lock->FindRow([](const test::cfg::Server& row) -> bool {
            return row.host() == "127.0.0.1";
        });

        logger().LOG_INFO("test::cfg::Server:\n{}", config_lock->DebugString());

        co_return nullptr;
    }

private:
    million::ServiceHandle config_service_;
    config::ConfigTableWeak<test::cfg::Server> tb_server_;
};

class TestApp : public million::IMillion {
    //using Base = million::IMillion;
    //using Base::Base;
};

int main() {
    auto test_app = std::make_unique<TestApp>();
    if (!test_app->Init("config_test_settings.yaml")) {
        return 0;
    }
    test_app->Start();

    auto service_opt = test_app->NewService<TestService>();
    if (!service_opt) {
        return 0;
    }
    auto service_handle = *service_opt;

    

    //for (int i = 0; i < 10000; i++) {
    //    auto msg2 = std::make_unique<Test1Msg>();
    //    test_app->Timeout(1, service_handle, std::move(msg2));
    //}

    //std::this_thread::sleep_for(std::chrono::seconds(10000));

    return 0;
}
