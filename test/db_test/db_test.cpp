#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <db/db.h>
#include <db/sql.h>

#include <protogen/db/db_options.pb.h>
#include <protogen/db/db_example.pb.h>

MILLION_MODULE_INIT();

namespace Db = Million::Proto::Db;
namespace protobuf = google::protobuf;

MILLION_MSG_DEFINE_EMPTY(, Test1Msg)

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
        //auto res = co_await Call<million::db::DbProtoRegisterMsg>(db_service_, "db/db_example.proto", false);
        //if (res->success) {
        //    logger().Info("DbProtoRegisterMsg success: {}.", "db/db_example.proto");
        //}

        //auto handle = imillion().GetServiceByName("SqlService");

        //auto user = std::make_unique<Db::Example::User>();
        //user->set_id(100);
        //user->set_password_hash("sadawd");
        //user->set_is_active(true);
        //user->set_created_at(10000);
        //user->set_updated_at(13123);
        //user->set_email("sb@qq.com");

        //auto row = million::db::DbRow(std::move(user));
        //// co_await Call<million::db::SqlInsertMsg>(*handle, million::make_nonnull(&row));

        //auto res2 = co_await Call<million::db::DbRowGetMsg>(db_service_, *Db::Example::User::GetDescriptor(), "103", std::nullopt);
        //if (!res2->db_row) {
        //    logger().Info("DbRowGetMsg failed.");
        //}



        co_return;
    }


    MILLION_MSG_DISPATCH(TestService);

    MILLION_MSG_HANDLE(Test1Msg, msg) {
        std::printf("?? ");
        //auto msg2 = std::make_unique<Test1Msg>();
        //Timeout(100, std::move(msg2));

        co_return;
    }


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

    //test_app->Send<Test1Msg>(service_handle, service_handle);

    for (int i = 0; i < 10000; i++) {
        auto msg2 = std::make_unique<Test1Msg>();
        test_app->Timeout(1, service_handle, std::move(msg2));
    }

    return 0;
}
