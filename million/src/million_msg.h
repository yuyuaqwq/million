#pragma once

#include <vector>

#include <million/imillion.h>

namespace million {

MILLION_MSG_DEFINE_EMPTY(, MillionServiceStartMsg)
MILLION_MSG_DEFINE_EMPTY(, MillionServiceStopMsg)
MILLION_MSG_DEFINE(, MillionSessionTimeoutMsg, (SessionId) timeout_id)

} // namespace million