#include <million/imillion.h>

#include "million.h"

namespace million {

IMillion::IMillion() {
    auto million = new Million(this);
    impl_ = million;
}

IMillion::~IMillion() {
    delete impl_;
}

bool IMillion::Start(std::string_view config_path) {
    auto success = impl_->Init(config_path);
    if (!success) {
        return false;
    }
    impl_->Start();
    return true;
}


std::optional<ServiceHandle> IMillion::AddService(std::unique_ptr<IService> iservice, bool start) {
    return impl_->AddService(std::move(iservice), start);
}


SessionId IMillion::StartService(const ServiceHandle& service_handle) {
    return impl_->StartService(service_handle);
}

SessionId IMillion::StopService(const ServiceHandle& service_handle) {
    return impl_->StopService(service_handle);
}


bool IMillion::SetServiceName(const ServiceHandle& handle, const ServiceName& name) {
    return impl_->SetServiceName(handle, name);
}

std::optional<ServiceHandle> IMillion::GetServiceByName(const ServiceName& name) {
    return impl_->GetServiceByName(name);
}


SessionId IMillion::NewSession() {
    return impl_->NewSession();
}


SessionId IMillion::Send(const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg) {
    return impl_->Send(sender, target, std::move(msg));
}

SessionId IMillion::SendTo(const ServiceHandle& sender, const ServiceHandle& target, SessionId session_id, MsgUnique msg) {
    return impl_->Send(sender, target, session_id, std::move(msg));
}


const YAML::Node& IMillion::YamlConfig() const {
    return impl_->YamlConfig();
}

void IMillion::Timeout(uint32_t tick, const ServiceHandle& service, MsgUnique msg) {
    return impl_->Timeout(tick, service, std::move(msg));
}

asio::io_context& IMillion::NextIoContext() {
    return impl_->NextIoContext();
}

Logger& IMillion::logger() {
    return impl_->logger();
}

void IMillion::EnableSeparateWorker(const ServiceHandle& service) {
    impl_->EnableSeparateWorker(service);
}

} //namespace million