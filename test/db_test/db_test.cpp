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

MILLION_MESSAGE_DEFINE_EMPTY(, Test1Msg)

class TestService : public million::IService {
    MILLION_SERVICE_DEFINE(TestService);

public:
    using Base = million::IService;
    TestService(million::IMillion* imillion)
        : Base(imillion) {}

    virtual bool OnInit() override {
        
        auto handle = imillion().FindServiceByName(db::kDBServiceName);
        if (!handle) {
            logger().LOG_ERROR("Unable to find DbService.");
            return false;
        }
        db_service_ = *handle;

        handle = imillion().FindServiceByName(db::kSqlServiceName);
        if (!handle) {
            logger().LOG_ERROR("Unable to find SqlService.");
            return false;
        }
        sql_service_ = *handle;

        return true;
    }

    virtual million::Task<million::MessagePointer> OnStart(::million::ServiceHandle sender, ::million::SessionId session_id, million::MessagePointer with_msg) override {
        auto init_resp = co_await Call<db::SqlTableInitReq>(sql_service_, *million::db::example::User::GetDescriptor());
        
        auto user = million::make_proto_message<million::db::example::User>();
        user->set_id(100);
        user->set_password_hash("sadawd");
        user->set_is_active(true);
        user->set_created_at(10000);
        user->set_updated_at(13123);
        user->set_email("sb@qq.com");
        // auto res = co_await Call<db::DBRowCreateReq>(db_service_, std::move(user));

        auto res2 = co_await Call<db::DBRowLoadReq, db::DBRowLoadResp>(db_service_
            , *million::db::example::User::GetDescriptor(), million::db::example::User::kIdFieldNumber, 103);
        if (!res2->db_row) {
            logger().LOG_INFO("DbRowGetMsg failed.");
        }

        res2->db_row->MarkDirty();
        co_await res2->db_row->Commit(this, db_service_);


        // auto handle = imillion().GetServiceByName(db::kDbServiceName);
        // co_await Call<db::SqlInsertMsg>(*handle, &row);

        co_return nullptr;
    }


    MILLION_MESSAGE_HANDLE(Test1Msg, msg) {
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
    if (!test_app->Init("db_test_settings.yaml")) {
        return 0;
    }
    test_app->Start();

    auto service_opt = test_app->NewService<TestService>();
    if (!service_opt) {
        return 0;
    }
    auto service_handle = *service_opt;

    // test_app->Send<Test1Msg>(service_handle, service_handle);
    //for (int i = 0; i < 10000; i++) {
    //    auto msg2 = std::make_unique<Test1Msg>();
    //    test_app->Timeout(1, service_handle, std::move(msg2));
    //}

    std::this_thread::sleep_for(std::chrono::seconds(10000));

    return 0;
}
