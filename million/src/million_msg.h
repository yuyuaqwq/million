#pragma once

#include <vector>

#include <million/imillion.h>

namespace million {

MILLION_MSG_DEFINE_EMPTY(MILLION_CLASS_API, MillionServiceExitMsg)
MILLION_MSG_DEFINE(MILLION_CLASS_API, MillionSessionTimeoutMsg, (SessionId) timeout_id)

} // namespace million