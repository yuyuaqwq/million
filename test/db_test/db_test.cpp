#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <db/db_row.h>

#include <protogen/db/db_example.pb.h>

namespace Db = Million::Proto::Db;
namespace protobuf = google::protobuf;

class TestService : public million::IService {
public:
    using Base = million::IService;
    using Base::Base;

    virtual bool OnInit() override {
        

        return true;
    }

    MILLION_MSG_DISPATCH(TestService);


private:
    million::ServiceHandle db_;

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
