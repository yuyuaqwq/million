#include <iostream>

#include "million/imillion.h"
#include "million/imsg.h"

#include <yaml-cpp/yaml.h>
#include <hiredis/async.h>

// 缓存服务
class CacheService : public million::IService {
public:
    using Base = million::IService;
    using Base::Base;

    bool Connect() {
        Close();
        connection_ = redisAsyncConnect("127.0.0.1", 6379);
        if (connection_ == nullptr || connection_->err) {
            if (connection_) {
                std::cerr << "Error: " << connection_->errstr << std::endl;
            }
            else {
                std::cerr << "Can't allocate redis context" << std::endl;
            }
            return false;
        }
        return true;
    }

    void Close() {
        if (connection_) {
            redisAsyncFree(connection_);
            connection_ = nullptr;;
        }
    }

    virtual void OnInit() override {
        if (!Connect()) {
            return;
        }

        // 执行一个简单的 SET 命令
        auto res = redisAsyncCommand(connection_, [](struct redisAsyncContext* c, void* r, void* privdata) {
                redisReply* reply = static_cast<redisReply*>(r);
                if (reply == nullptr) return;
                std::cout << "SET: " << reply->str << std::endl;
                freeReplyObject(reply);
            }
            , nullptr, "SET %s %s", "key", "hello");


        // 执行一个 GET 命令
        res = redisAsyncCommand(connection_, [](struct redisAsyncContext* c, void* r, void* privdata) {
                redisReply* reply = static_cast<redisReply*>(r);
                if (reply == nullptr) return;
                std::cout << "GET key: " << reply->str << std::endl;
                freeReplyObject(reply);
            }
            , nullptr, "GET %s", "key");


    }

    virtual million::Task OnMsg(million::MsgUnique msg) override {
        co_return;
    }

    virtual void OnExit() override {
        Close();
    }



private:
    redisAsyncContext* connection_ = nullptr;
};


MILLION_FUNC_EXPORT bool MillionModuleInit(million::IMillion* imillion) {
    auto& config = imillion->config();
    auto service_handle = imillion->NewService<CacheService>();

    return true;
}
