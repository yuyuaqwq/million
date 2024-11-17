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

bool IMillion::SetServiceUniqueName(const ServiceHandle& handle, const ServiceUniqueName& unique_name) {
    return million_->SetServiceUniqueName(handle, unique_name);
}

std::optional<ServiceHandle> IMillion::GetServiceByUniqueNum(const ServiceUniqueName& unique_name) {
    return million_->GetServiceByUniqueNum(unique_name);
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

void IMillion::Log(const ServiceHandle& sender, logger::LogLevel level, const char* file, int line, const char* function, std::string_view str) {
    million_->Log(sender, level, file, line, function, str);
}

void IMillion::EnableSeparateWorker(const ServiceHandle& service) {
    million_->EnableSeparateWorker(service);
}

} //namespace million