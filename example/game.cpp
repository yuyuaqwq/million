#include "milinet/milinet.h"
#include "milinet/service.h"


int main() {
    milinet::Milinet net;

    auto service = std::make_unique<milinet::Service>(net.AllocServiceId());
    auto& service_ref = net.AddService(std::move(service));

    net.Start();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    net.Send(&service_ref, std::make_unique<milinet::Msg>());
    net.Send(&service_ref, std::make_unique<milinet::Msg>());
    net.Send(&service_ref, std::make_unique<milinet::Msg>());
    net.Send(&service_ref, std::make_unique<milinet::Msg>());
    net.Send(&service_ref, std::make_unique<milinet::Msg>());
    net.Send(&service_ref, std::make_unique<milinet::Msg>());

    std::this_thread::sleep_for(std::chrono::seconds(100));

    return 0;
}