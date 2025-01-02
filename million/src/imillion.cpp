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

bool IMillion::Init(std::string_view config_path) {
    return impl_->Init(config_path);
}

void IMillion::Start() {
    impl_->Start();
}


std::optional<ServiceHandle> IMillion::AddService(std::unique_ptr<IService> iservice, MsgUnique init_msg) {
    auto shared = impl_->AddService(std::move(iservice), std::move(init_msg));
    if (!shared) {
        return std::nullopt;
    }
    return ServiceHandle(*shared);
}


std::optional<SessionId> IMillion::StartService(const ServiceHandle& service) {
    auto lock = service.lock();
    if (!lock) {
        return std::nullopt;
    }
    return impl_->StartService(lock);
}

std::optional<SessionId> IMillion::StopService(const ServiceHandle& service) {
    auto lock = service.lock();
    if (!lock) {
        return std::nullopt;
    }
    return impl_->StopService(lock);
}


bool IMillion::SetServiceName(const ServiceHandle& service, const ServiceName& name) {
    auto lock = service.lock();
    if (!lock) {
        return false;
    }
    return impl_->SetServiceName(lock, name);
}

std::optional<ServiceHandle> IMillion::GetServiceByName(const ServiceName& name) {
    auto shared = impl_->GetServiceByName(name);
    if (!shared) {
        return std::nullopt;
    }
    return ServiceHandle(*shared);
}


SessionId IMillion::NewSession() {
    return impl_->NewSession();
}


std::optional<SessionId> IMillion::Send(const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg) {
    auto sender_lock = sender.lock();
    if (!sender_lock) {
        return std::nullopt;
    }
    auto target_lock = target.lock();
    if (!target_lock) {
        return std::nullopt;
    }
    return impl_->Send(sender_lock, target_lock, std::move(msg));
}

bool IMillion::SendTo(const ServiceHandle& sender, const ServiceHandle& target, SessionId session_id, MsgUnique msg) {
    auto sender_lock = sender.lock();
    if (!sender_lock) {
        return false;
    }
    auto target_lock = target.lock();
    if (!target_lock) {
        return false;
    }
    return impl_->SendTo(sender_lock, target_lock, session_id, std::move(msg));
}


const YAML::Node& IMillion::YamlConfig() const {
    return impl_->YamlConfig();
}

bool IMillion::Timeout(uint32_t tick, const ServiceHandle& service, MsgUnique msg) {
    auto lock = service.lock();
    if (!lock) {
        return false;
    }
    impl_->Timeout(tick, lock, std::move(msg));
    return true;
}

asio::io_context& IMillion::NextIoContext() {
    return impl_->NextIoContext();
}

Logger& IMillion::logger() {
    return impl_->logger();
}

ProtoMgr& IMillion::proto_mgr() {
    return impl_->proto_mgr();
}

void IMillion::EnableSeparateWorker(const ServiceHandle& handle) {
    auto lock = handle.lock();
    if (!lock) {
        return;
    }
    impl_->EnableSeparateWorker(lock);
}

} //namespace million