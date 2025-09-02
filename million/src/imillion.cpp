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

bool IMillion::Init(std::string_view settings_path) {
    return impl_->Init(settings_path);
}

void IMillion::Start() {
    impl_->Start();
}

std::optional<ServiceHandle> IMillion::AddService(std::unique_ptr<IService> iservice) {
    auto shared = impl_->AddService(std::move(iservice));
    if (!shared) {
        return std::nullopt;
    }
    return ServiceHandle(*shared);
}

std::optional<SessionId> IMillion::StartService(const ServiceHandle& service, MessagePointer with_msg) {
    auto lock = service.lock();
    if (!lock) {
        return std::nullopt;
    }
    return impl_->StartService(lock, std::move(with_msg));
}

std::optional<SessionId> IMillion::StopService(const ServiceHandle& service, MessagePointer with_msg) {
    auto lock = service.lock();
    if (!lock) {
        return std::nullopt;
    }
    return impl_->StopService(lock, std::move(with_msg));
}

std::optional<ServiceHandle> IMillion::FindServiceById(ServiceId service_id) {
    auto shared = impl_->FindServiceById(service_id);
    if (!shared) {
        return std::nullopt;
    }
    return ServiceHandle(*shared);
}

bool IMillion::SetServiceName(const ServiceHandle& service, const ServiceName& name) {
    auto lock = service.lock();
    if (!lock) {
        return false;
    }
    return impl_->SetServiceName(lock, name);
}

std::optional<ServiceHandle> IMillion::FindServiceByName(const ServiceName& name) {
    auto shared = impl_->FindServiceByName(name);
    if (!shared) {
        return std::nullopt;
    }
    return ServiceHandle(*shared);
}

SessionId IMillion::NewSession() {
    return impl_->NewSession();
}

std::optional<SessionId> IMillion::Send(const ServiceHandle& sender, const ServiceHandle& target, MessagePointer msg) {
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

bool IMillion::SendTo(const ServiceHandle& sender, const ServiceHandle& target, SessionId session_id, MessagePointer msg) {
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

const YAML::Node& IMillion::YamlSettings() const {
    return impl_->YamlSettings();
}

bool IMillion::Timeout(uint32_t tick, const ServiceHandle& service, MessagePointer msg) {
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

NodeId IMillion::node_id() {
    return impl_->node_id();
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