#pragma once

#include <vector>

#include <google/protobuf/message.h>

#include <million/imsg.h>

namespace million {

using ProtoMsgUnique = std::unique_ptr<google::protobuf::Message>;
using Buffer = std::vector<uint8_t>;

} // namespace million