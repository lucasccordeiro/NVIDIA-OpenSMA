// ESBMC verification stub for nv/logger/log.h
// All logging calls are no-ops; log data is not part of the verification goal.
#pragma once
#include "nv/logger/common.h"

namespace nv::logger {

// Match the production signatures: take EventStructItem by value so that
// brace-initializer second arguments (EventData = std::array<uint8_t,8>)
// are accepted by overload resolution rather than a deduced variadic pack.
inline void info_wait(EventStructItem, EventData = {}) {}
inline void error_wait(EventStructItem, EventData = {}) {}
inline void info(EventStructItem, EventData = {}) {}
inline void error(EventStructItem, EventData = {}) {}

}  // namespace nv::logger
