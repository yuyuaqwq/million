#pragma once

#include <vector>

namespace million {
namespace net {

using Packet = std::vector<uint8_t>;

constexpr uint32_t kPacketMaxSize = 1024 * 1024 * 64;


} // namespace net
} // namespace million