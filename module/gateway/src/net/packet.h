#pragma once

#include <vector>

#include <million/proto_msg.h>

namespace million {
namespace net {

using Packet = Buffer;

constexpr uint32_t kPacketMaxSize = 1024 * 1024 * 64;
constexpr uint32_t kPacketRecvQueueMaxCount = 1024;  // Œ¥ π”√

} // namespace net
} // namespace million