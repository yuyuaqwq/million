#pragma once

#include <google/protobuf/message.h>

#include <million/imsg.h>

namespace million {

using ProtoMsgUnique = std::unique_ptr<google::protobuf::Message>;

} // namespace million