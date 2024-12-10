#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <db/db.h>

#include <protogen/db/db_options.pb.h>
#include <protogen/db/db_example.pb.h>

namespace Db = Million::Proto::Db;
namespace protobuf = google::protobuf;

class TestService : public million::IService {
public:
    using Base = million::IService;
    using Base::Base;

    virtual bool OnInit() override {
        
        auto handle = imillion().GetServiceByName("DbService");
        if (!handle) {
            logger().Err("Unable to find SqlService.");
            return false;
        }
        db_service_ = *handle;

        return true;
    }

    virtual million::Task<> OnStart() override {
        auto res = co_await Call<million::db::DbProtoRegisterMsg>(db_service_, "db/db_example.proto", false);
        if (res->success) {
            logger().Info("DbProtoRegisterMsg success: {}.", "db/db_example.proto");
        }
        co_return;
    }


    MILLION_MSG_DISPATCH(TestService);


private:
    million::ServiceHandle db_service_;

};

class TestApp : public million::IMillion {
    //using Base = million::IMillion;
    //using Base::Base;
};

int main() {
    auto test_app = std::make_unique<TestApp>();
    if (!test_app->Init("db_test_config.yaml")) {
        return 0;
    }

    auto service_opt = test_app->NewService<TestService>();
    if (!service_opt) {
        return 0;
    }
    auto service_handle = *service_opt;


    return 0;
}
