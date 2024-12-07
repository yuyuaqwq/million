#include <million/imillion.h>

#include "million.h"

namespace million {

IMillion::IMillion() {
    auto million = new Million(this);
    million_ = million;
}

IMillion::~IMillion() {
    delete million_;
}

bool IMillion::Init(std::string_view config_path) {
    auto success = million_->Init(config_path);
    if (!success) {
        return false;
    }
    million_->Start();
    return true;
}

std::optional<ServiceHandle> IMillion::AddService(std::unique_ptr<IService> iservice) {
    return million_->AddService(std::move(iservice));
}

void IMillion::DeleteService(ServiceHandle&& service_handle) {
    million_->DeleteService(std::move(service_handle));
}

bool IMillion::SetServiceName(const ServiceHandle& handle, const ServiceName& name) {
    return million_->SetServiceName(handle, name);
}

std::optional<ServiceHandle> IMillion::GetServiceByName(const ServiceName& name) {
    return million_->GetServiceByName(name);
}

SessionId IMillion::AllocSessionId() {
    return million_->AllocSessionId();
}

SessionId IMillion::Send(const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg) {
    return million_->Send(sender, target, std::move(msg));
}

SessionId IMillion::Send(SessionId session_id, const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg) {
    return million_->Send(session_id, sender, target, std::move(msg));
}

const YAML::Node& IMillion::YamlConfig() const {
    return million_->YamlConfig();
}

void IMillion::Timeout(uint32_t tick, const ServiceHandle& service, MsgUnique msg) {
    return million_->Timeout(tick, service, std::move(msg));
}

asio::io_context& IMillion::NextIoContext() {
    return million_->NextIoContext();
}

Logger& IMillion::logger() {
    return million_->logger();
}

void IMillion::EnableSeparateWorker(const ServiceHandle& service) {
    million_->EnableSeparateWorker(service);
}

} //namespace million