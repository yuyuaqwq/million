#pragma once

#include <iostream>
#include <queue>
#include <any>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <million/imillion.h>
#include <million/proto_msg.h>

namespace million {
namespace db {

enum class DbMsgType : uint32_t {
    kDbQuery,
};
using DbMsgBase = million::MsgBaseT<DbMsgType>;

MILLION_MSG_DEFINE(DbQueryMsg, DbMsgType::kDbQuery, (million::ProtoMsgUnique) proto_msg);

class DbService : public million::IService {
public:
    using Base = million::IService;
    using Base::Base;

    virtual void OnInit() override {
        
    }

    virtual million::Task OnMsg(million::MsgUnique msg) override {

        co_return;
    }

    virtual void OnExit() override {
        
    }

    // 返回一个Protobuf msg
    // 首先OnMsg收到Query消息，然后Call Cache的Query，有的话返回，没有的话Call Sql的Query
    void Query() {

    }

private:

};

} // namespace db
} // namespace million