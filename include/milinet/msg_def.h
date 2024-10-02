#pragma once

#include <cstdint>

#include <memory>

namespace milinet {

using SessionId = uint64_t;

constexpr SessionId kSessionIdInvalid = -1;

class Msg;
using MsgUnique = std::unique_ptr<Msg>;

} // namespace milinet