#include <iostream>

#include <million/imillion.h>
#include <million/imsg.h>

#include <yaml-cpp/yaml.h>
#include <hiredis.h>

// 缓存服务
class CacheService : public million::IService {
public:
    using Base = million::IService;
    using Base::Base;

    virtual void OnInit() override {
        if (!Connect()) {
            return;
        }

        // 执行一个简单的 SET 命令
        auto reply = static_cast<redisReply*>(redisCommand(connection_, "SET %s %s", "key", "hello"));
        std::cout << "SET: " << reply->str << std::endl;
        freeReplyObject(reply);

        // 执行一个 GET 命令
        reply = static_cast<redisReply*>(redisCommand(connection_, "GET %s", "key"));
        std::cout << "GET key: " << reply->str << std::endl;
        freeReplyObject(reply);
    }

    virtual million::Task OnMsg(million::MsgUnique msg) override {
        co_return;
    }

    virtual void OnExit() override {
        Close();
    }

    bool Connect() {
        Close();
        connection_ = redisConnect("127.0.0.1", 6379);
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
            redisFree(connection_);
            connection_ = nullptr;;
        }
    }

private:
    redisContext* connection_ = nullptr;
};


MILLION_FUNC_EXPORT bool MillionModuleInit(million::IMillion* imillion) {
    auto& config = imillion->config();
    auto service_handle = imillion->NewService<CacheService>();

    return true;
}
