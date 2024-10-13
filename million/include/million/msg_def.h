#pragma once

#include <cstdint>

#include <memory>

namespace million {

using SessionId = uint64_t;

constexpr SessionId kSessionIdInvalid = -1;

class IMsg;
using MsgUnique = std::unique_ptr<IMsg>;

} // namespace million