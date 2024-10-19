#pragma once

#include <iostream>
#include <queue>
#include <any>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <million/imillion.h>
#include <million/imsg.h>
#include <million/proto_msg.h>

enum DbMsgType {
    kQuery,
};

using DbMsgBase = million::MsgBaseT<DbMsgType>;

struct DbMsgQuery : million::MsgT<kQuery> {
    DbMsgQuery(auto&& msg)
        : proto_msg(std::move(msg)) { }

    million::ProtoMsgUnique proto_msg;
};


class DbService : public million::IService {
public:
    using Base = million::IService;
    using Base::Base;

    virtual void OnInit() override {
        
    }

    virtual million::Task OnMsg(million::MsgUnique msg) override {
        MILLION_HANDLE_MSG_BEGIN(std::move(msg), DbMsgBase);

        MILLION_HANDLE_MSG(msg, DbMsgQuery, {
            
        });

        MILLION_HANDLE_MSG_END();

        co_return;
    }

    virtual void OnExit() override {
        // Close();
    }

    // ����һ��Protobuf msg
    // ����OnMsg�յ�Query��Ϣ��Ȼ��Call Cache��Query���еĻ����أ�û�еĻ�Call Sql��Query
    void Query() {

    }

private:

};
