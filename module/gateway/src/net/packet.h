#pragma once

#include <vector>

namespace million {
namespace net {

using Packet = std::vector<uint8_t>;

constexpr uint32_t kPacketMaxSize = 1024 * 1024 * 64;
constexpr uint32_t kPacketRecvQueueMaxCount = 1024;  // δʹ��

} // namespace net
} // namespace million