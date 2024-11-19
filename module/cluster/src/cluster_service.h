#pragma once

#include <million/iservice.h>

#include "cluster_server.h"

namespace million {
namespace cluster {

class ClusterService : public IService {
public:
    using Base = IService;
    using Base::Base;

    virtual bool OnInit() override {
        return true;
    }

    virtual Task<> OnMsg(MsgUnique msg) override {
        co_return;
    }

private:

};

} // namespace gateway
} // namespace million
