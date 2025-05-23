#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <config/config.h>
#include <config/cfg_example.pb.h>

MILLION_MODULE_INIT();

namespace protobuf = google::protobuf;
namespace config = million::config;
namespace example = config::example;

MILLION_MESSAGE_DEFINE_EMPTY(, Test1Msg)

class TestService : public million::IService {
    MILLION_SERVICE_DEFINE(TestService);

public:
    using Base = million::IService;
    TestService(million::IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit() override {
        auto handle = imillion().GetServiceByName(config::kConfigServiceName);
        if (!handle) {
            logger().LOG_ERROR("Unable to find ConfigService.");
            return false;
        }
        config_service_ = *handle;

        return true;
    }

    virtual million::Task<million::MessagePointer> OnStart(million::ServiceHandle sender, million::SessionId session_id, million::MessagePointer with_msg) override {
        example_kv_config_ = co_await config::QueryConfig<config::example::ExampleKV>(this, config_service_);
        
        Send<Test1Msg>(service_handle());

        co_return nullptr;
    }


    MILLION_MESSAGE_HANDLE(Test1Msg, msg) {
        auto config_lock = co_await example_kv_config_.Lock(this, config_service_);

        million::ProtoFieldAny f;

        config_lock->BuildIndex(config::example::ExampleKV::kServerPortFieldNumber);
        config_lock->BuildIndex(config::example::ExampleKV::kServerIPFieldNumber);

        auto aa = config_lock->FindRowByIndex(config::example::ExampleKV::kServerPortFieldNumber, 1024u);
        auto bb = config_lock->FindRowByIndex(config::example::ExampleKV::kServerIPFieldNumber, "8.8.8.8");

        config_lock->BuildCompositeIndex({ config::example::ExampleKV::kServerPortFieldNumber, config::example::ExampleKV::kServerIPFieldNumber });
        auto aabb = config_lock->FindRowByCompositeIndex({ config::example::ExampleKV::kServerPortFieldNumber, config::example::ExampleKV::kServerIPFieldNumber }
            , 1024u, "8.8.8.8");


        auto row = config_lock->FindRow([](const config::example::ExampleKV& row) -> bool {
            return row.serverip() == "8.8.8.8";
        });

        logger().LOG_INFO("ExampleKV:\n{}", config_lock->DebugString());

        auto example_data_config = co_await config::QueryConfig<config::example::ExampleData>(this, config_service_);
        auto config_lock2 = co_await example_data_config.Lock(this, config_service_);
        logger().LOG_INFO("ExampleData:\n{}", config_lock2->DebugString());

        co_return nullptr;
    }

private:
    million::ServiceHandle config_service_;
    million::config::ConfigTableWeak<config::example::ExampleKV> example_kv_config_;
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

    std::this_thread::sleep_for(std::chrono::seconds(10000));

    return 0;
}
