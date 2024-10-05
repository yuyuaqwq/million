#include "iostream"

#include "milinet/imilinet.hpp"
#include "milinet/imsg.hpp"

#include <asio.hpp>


class TestMsg : public milinet::IMsg {
public:
    TestMsg(int value1, std::string_view value2)
        : value1(value1)
        , value2(value2) {}

public:
    int value1;
    std::string value2;
};

class TestService : public milinet::IService {
    using Base = milinet::IService;
    using Base::Base;

    virtual milinet::Task OnMsg(milinet::MsgUnique msg) override {
        auto test_msg = static_cast<TestMsg*>(msg.get());
        std::cout << test_msg->session_id() << test_msg->value1 << test_msg->value2 << std::endl;

        auto res = co_await Recv<milinet::IMsg>(2);
        std::cout << res->session_id() << std::endl;

        res = co_await Recv<milinet::IMsg>(3);
        std::cout << res->session_id() << std::endl;

        co_await On4();
        co_await On5();
        co_return;
    }

    milinet::Task On4() {
        auto res = co_await Recv<milinet::IMsg>(4);
        std::cout << res->session_id() << std::endl;
        co_return;
    }

    milinet::Task On5() {
        auto session_id = Send<TestMsg>(service_handle(), 5, std::string_view("hjh"));
        auto res = co_await Recv<milinet::IMsg>(session_id);
        std::cout << res->session_id() << std::endl;
        co_return;
    }
};



asio::awaitable<void> echo(asio::ip::tcp::socket socket)
{
    try
    {
        char data[1024];
        for (;;)
        {
            std::size_t n = co_await socket.async_read_some(asio::buffer(data), asio::use_awaitable);
            co_await asio::async_write(socket, asio::buffer(data, n), asio::use_awaitable);
        }
    }
    catch (std::exception& e)
    {
        std::printf("echo Exception: %s\n", e.what());
    }
}

asio::awaitable<void> listener()
{
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor(executor, { asio::ip::tcp::v4(), 10086 });
    for (;;)
    {
        asio::ip::tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(executor, echo(std::move(socket)), asio::detached);
    }
}

void Loop() {
    asio::io_context io_context(1);
    asio::co_spawn(io_context, listener(), asio::detached);
    io_context.run();
}

MILINET_FUNC_EXPORT bool MiliModuleInit(milinet::IMilinet* imilinet) {
    Loop();
    
    auto service_handle = imilinet->CreateService<TestService>();
     
    imilinet->Send<TestMsg>(service_handle, 666, std::string_view("sb"));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    imilinet->Send<TestMsg>(service_handle, 2, "6");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    imilinet->Send<TestMsg>(service_handle, 3, "emm");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    imilinet->Send<TestMsg>(service_handle, 4, "hhh");

    std::this_thread::sleep_for(std::chrono::seconds(1));

    return true;
}
