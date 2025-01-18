#include <iostream>

#include <million/imillion.h>

#include <yaml-cpp/yaml.h>

#include <db/db.h>
#include <db/sql.h>

#include <db/db_options.pb.h>
#include <db/db_example.pb.h>

MILLION_MODULE_INIT();

namespace protobuf = google::protobuf;
namespace db = million::db;

MILLION_MSG_DEFINE_EMPTY(, Test1Msg)

class TestService : public million::IService {
public:
    using Base = million::IService;
    TestService(million::IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit(million::MsgPtr msg) override {
        
        auto handle = imillion().GetServiceByName("DbService");
        if (!handle) {
            logger().Err("Unable to find DbService.");
            return false;
        }
        db_service_ = *handle;

        handle = imillion().GetServiceByName("SqlService");
        if (!handle) {
            logger().Err("Unable to find SqlService.");
            return false;
        }
        sql_service_ = *handle;

        return true;
    }

    virtual million::Task<million::MsgPtr> OnStart(::million::ServiceHandle sender, ::million::SessionId session_id) override {
        // co_await Call<db::SqlTableInitMsg>(sql_service_, *million::db::example::User::GetDescriptor());
        
        auto user = million::make_proto_msg<million::db::example::User>();
        user->set_id(100);
        user->set_password_hash("sadawd");
        user->set_is_active(true);
        user->set_created_at(10000);
        user->set_updated_at(13123);
        user->set_email("sb@qq.com");
        // auto res = co_await Call<db::DbRowCreateMsg>(db_service_, std::move(user));

        auto res2 = co_await Call<db::DbRowQueryMsg>(db_service_, *million::db::example::User::GetDescriptor(), "103"
            , std::nullopt, true);
        if (!res2->db_row) {
            logger().Info("DbRowGetMsg failed.");
        }

        res2->db_row->MarkDirty();
        auto res3 = co_await Call<db::DbRowUpdateMsg>(db_service_, &*res2->db_row);


        // auto handle = imillion().GetServiceByName("SqlService");
        // co_await Call<db::SqlInsertMsg>(*handle, &row);

        co_return nullptr;
    }


    MILLION_MSG_DISPATCH(TestService);

    MILLION_MSG_HANDLE(Test1Msg, msg) {
        std::printf("?? ");
        //auto msg2 = std::make_unique<Test1Msg>();
        //Timeout(100, std::move(msg2));

        co_return nullptr;
    }

private:
    million::ServiceHandle db_service_;
    million::ServiceHandle sql_service_;

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
    test_app->Start();

    auto service_opt = test_app->NewService<TestService>();
    if (!service_opt) {
        return 0;
    }
    auto service_handle = *service_opt;

    //test_app->Send<Test1Msg>(service_handle, service_handle);
    //for (int i = 0; i < 10000; i++) {
    //    auto msg2 = std::make_unique<Test1Msg>();
    //    test_app->Timeout(1, service_handle, std::move(msg2));
    //}

    std::this_thread::sleep_for(std::chrono::seconds(10000));

    return 0;
}
