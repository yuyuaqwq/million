#pragma once

#include <cstdint>

#include <million/seata_snowflake.hpp>

namespace million {

using SessionId = SnowId;
constexpr SessionId kSessionInvalidId = kSnowInvalidId;

constexpr uint32_t kSessionNeverTimeout = 0xffffffff;

inline SessionId SessionSendToReplyId(SessionId session_id) {
	return session_id | 0x8000000000000000;
}

inline SessionId SessionReplyToSendId(SessionId session_id) {
	return session_id & 0x7fffffffffffffff;
}

inline bool SessionIsReplyId(SessionId session_id) {
	return session_id >> 63;
}

inline bool SessionIsSendId(SessionId session_id) {
	return !(session_id >> 63);
}

} // namespace million