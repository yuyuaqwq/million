#pragma once

#include <cstdint>

#include <memory>

#include "million/imillion.h"

namespace million {

class ConfigException : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message)
        : std::runtime_error("config error: " + message) {}
};

class ServiceMgr;
class MsgMgr;
class ModuleMgr;
class WorkerMgr;
class Million : public IMillion {
public:
    Million(std::string_view config_path);
    ~Million();

    void Init();
    void Start();

    virtual ServiceHandle AddService(std::unique_ptr<IService> iservice) override;
    using IMillion::MakeService;

    virtual SessionId Send(ServiceHandle target, MsgUnique msg) override;
    using IMillion::Send;

    virtual const YAML::Node& config() const override;

    auto& service_mgr() { assert(service_mgr_); return *service_mgr_; }
    auto& msg_mgr() { assert(service_mgr_); return *msg_mgr_; }
    auto& module_mgr() { assert(module_mgr_); return *module_mgr_; }
    auto& worker_mgr() { assert(worker_mgr_); return *worker_mgr_; }

private:
    std::unique_ptr<YAML::Node> config_;

    std::unique_ptr<ServiceMgr> service_mgr_;
    std::unique_ptr<MsgMgr> msg_mgr_;
    std::unique_ptr<ModuleMgr> module_mgr_;
    std::unique_ptr<WorkerMgr> worker_mgr_;
};

} // namespace million