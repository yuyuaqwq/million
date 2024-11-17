#include <million/imillion.h>

#include "million.h"

namespace million {

IMillion::IMillion(std::string_view config_path) {
    auto million = new Million(this, config_path);
    million_ = million;
    million_->Init();
    million_->Start();
}

IMillion::~IMillion() {
    delete million_;
}

ServiceHandle IMillion::AddService(std::unique_ptr<IService> iservice) {
    return million_->AddService(std::move(iservice));
}

void IMillion::DeleteService(ServiceHandle&& service_handle) {
    million_->DeleteService(std::move(service_handle));
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