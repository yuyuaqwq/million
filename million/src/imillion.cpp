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
    auto shared = impl_->AddService(std::move(iservice), start);
    if (!shared) {
        return std::nullopt;
    }
    return ServiceHandle(*shared);
}


SessionId IMillion::StartService(const ServiceHandle& handle) {
    auto lock = handle.lock();
    if (!lock) {
        return kSessionIdInvalid;
    }
    return impl_->StartService(lock);
}

SessionId IMillion::StopService(const ServiceHandle& handle) {
    auto lock = handle.lock();
    if (!lock) {
        return kSessionIdInvalid;
    }
    return impl_->StopService(lock);
}


bool IMillion::SetServiceName(const ServiceHandle& handle, const ServiceName& name) {
    auto lock = handle.lock();
    if (!lock) {
        return kSessionIdInvalid;
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


SessionId IMillion::Send(const ServiceHandle& sender, const ServiceHandle& target, MsgUnique msg) {
    auto sender_lock = sender.lock();
    if (!sender_lock) {
        return kSessionIdInvalid;
    }
    auto target_lock = target.lock();
    if (!target_lock) {
        return kSessionIdInvalid;
    }

    return impl_->Send(sender_lock, target_lock, std::move(msg));
}

SessionId IMillion::SendTo(const ServiceHandle& sender, const ServiceHandle& target, SessionId session_id, MsgUnique msg) {
    auto sender_lock = sender.lock();
    if (!sender_lock) {
        return kSessionIdInvalid;
    }
    auto target_lock = target.lock();
    if (!target_lock) {
        return kSessionIdInvalid;
    }
    return impl_->SendTo(sender_lock, target_lock, session_id, std::move(msg));
}


const YAML::Node& IMillion::YamlConfig() const {
    return impl_->YamlConfig();
}

void IMillion::Timeout(uint32_t tick, const ServiceHandle& handle, MsgUnique msg) {
    auto lock = handle.lock();
    if (!lock) {
        return;
    }
    return impl_->Timeout(tick, lock, std::move(msg));
}

asio::io_context& IMillion::NextIoContext() {
    return impl_->NextIoContext();
}

Logger& IMillion::logger() {
    return impl_->logger();
}

void IMillion::EnableSeparateWorker(const ServiceHandle& handle) {
    auto lock = handle.lock();
    if (!lock) {
        return;
    }
    impl_->EnableSeparateWorker(lock);
}

} //namespace million