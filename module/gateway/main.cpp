#include "iostream"

#include <atomic>
#include <chrono>
#include <optional>
#include <mutex>
#include <queue>
#include <random>
#include <sstream>
#include <iomanip>

#include "million/imillion.h"
#include "million/imsg.h"

#include "net/server.h"

#include <yaml-cpp/yaml.h>


enum class NetMsgType {
    kRegister,
    kConnection,
    kSend,
    kRecv,
};

class NetMsg : public million::IMsg {
public:
    NetMsg(NetMsgType type)
        : type_(type) {}

    NetMsgType type() const { return type_; }

private:
    NetMsgType type_;
};

class RegisterMsg : public NetMsg {
public:
    RegisterMsg(million::ServiceHandle handle)
        : NetMsg(NetMsgType::kRegister)
        , handle_(handle) {}

    million::ServiceHandle handle() const { return handle_; }

private:
    million::ServiceHandle handle_;
};

class ConnectionMsg : public NetMsg {
public:
    ConnectionMsg()
        : NetMsg(NetMsgType::kConnection) {}

private:

};

class SendMsg : public NetMsg {
public:
    SendMsg()
        : NetMsg(NetMsgType::kSend) {}

private:

};

class RecvMsg : public NetMsg {
public:
    RecvMsg()
        : NetMsg(NetMsgType::kRecv) {}

public:

};


//struct NetPacket {
//
//    std::vector<uint8_t> buff;
//};


class TokenGenerator {
public:
    TokenGenerator()
        : gen_(rd_())
        , dis_(0, std::numeric_limits<uint32_t>::max()) {}

    uint64_t generate_token() {
        auto now = std::chrono::system_clock::now();
        uint32_t timestamp = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());

        uint32_t random_value = dis_(gen_);

        // 64位token
        uint64_t token = (static_cast<uint64_t>(timestamp) << 32) | random_value;
        return token;
    }

private:
    std::random_device rd_;
    std::mt19937_64 gen_;
    std::uniform_int_distribution<uint32_t> dis_;
};


class NetService : public million::IService {
public:
    using Base = million::IService;
    using Base::Base;

    virtual void OnInit() override {
        server_.set_on_connection([](auto connection_handle) {
            // 新连接到来
            std::cout << "on_connection" << std::endl;
        });
        server_.set_on_msg([](auto& connection, auto&& packet) {
            // 在这里向注册过的服务发送RecvMsg
            std::cout << "on_msg" << std::endl;
        });

        server_.Start(1, 8001);
    }

    virtual million::Task OnMsg(million::MsgUnique msg) override {
        auto netmsg = static_cast<NetMsg*>(msg.get());
        switch (netmsg->type()) {
        case NetMsgType::kRegister: {
            OnRegister(static_cast<RegisterMsg*>(netmsg));
            break;
        }
        case NetMsgType::kSend: {
            OnSend(static_cast<SendMsg*>(netmsg));
            break;
        }
        }
        co_return;
    }

    void OnRegister(RegisterMsg* msg) {
        std::lock_guard guard(register_service_mutex_);
        register_service_.emplace_back(msg->handle());
    }

    void OnSend(SendMsg* msg) {

    }

private:
    std::mutex register_service_mutex_;
    std::vector<million::ServiceHandle> register_service_;

    million::net::Server server_;
};


MILLION_FUNC_EXPORT bool MillionModuleInit(million::IMillion* imillion) {
    auto& config = imillion->config();

    auto service_handle = imillion->NewService<NetService>();

    return true;
}
