#pragma once

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include <api.h>

namespace million {

class MILLION_PROTOGEN_API ProtoMeta {
public:
    ProtoMeta(const google::protobuf::DescriptorPool* desc_pool
        , google::protobuf::DescriptorDatabase* desc_db
        , google::protobuf::MessageFactory* msg_factory)
        : desc_pool_(desc_pool)
        , desc_db_(desc_db)
        , msg_factory_(msg_factory) {}

    const google::protobuf::DescriptorPool& desc_pool() const { return *desc_pool_; }
    google::protobuf::DescriptorDatabase& desc_db() const { return *desc_db_; }
    google::protobuf::MessageFactory& msg_factory() const { return *msg_factory_; }

private:
    const google::protobuf::DescriptorPool* desc_pool_;
    google::protobuf::DescriptorDatabase* desc_db_;
    google::protobuf::MessageFactory* msg_factory_;
};

} // namespace million