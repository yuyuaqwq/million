#include "iostream"

#include <atomic>
#include <chrono>
#include <optional>
#include <mutex>
#include <queue>
#include <random>
#include <sstream>
#include <iomanip>

#include "milinet/imilinet.hpp"
#include "milinet/imsg.hpp"

#include <yaml-cpp/yaml.h>




enum class NetMsgType {
    kRegister,
    kConnection,
    kSend,
    kRecv,
};

class NetMsg : public milinet::IMsg {
public:
    NetMsg(NetMsgType type)
        : type_(type) {}

    NetMsgType type() const { return type_; }

private:
    NetMsgType type_;
};

class RegisterMsg : public NetMsg {
public:
    RegisterMsg(milinet::ServiceHandle handle)
        : NetMsg(NetMsgType::kRegister)
        , handle_(handle) {}

    milinet::ServiceHandle handle() const { return handle_; }

private:
    milinet::ServiceHandle handle_;
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

        // 将时间戳和随机数组合为 64 位的 token
        uint64_t token = (static_cast<uint64_t>(timestamp) << 32) | random_value;
        return token;
    }

private:
    std::random_device rd_;
    std::mt19937_64 gen_;
    std::uniform_int_distribution<uint32_t> dis_;
};


class NetService : public milinet::IService {
public:
    using Base = milinet::IService;
    using Base::Base;

    virtual void OnInit() override {
        
    }

    virtual milinet::Task OnMsg(milinet::MsgUnique msg) override {
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
    std::vector<milinet::ServiceHandle> register_service_;

};

// 向其他服务发接收网络消息，也通过OnMsg，需要先注册服务Handle
// 其他服务发网络消息，通过OnMsg接收


void MainLoop() {
    
}

MILINET_FUNC_EXPORT bool MiliModuleInit(milinet::IMilinet* imilinet) {
    auto& config = imilinet->config();
    
    auto service_handle = imilinet->MakeService<NetService>();

    return true;
}
