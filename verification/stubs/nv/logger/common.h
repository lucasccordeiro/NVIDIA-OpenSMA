// ESBMC verification stub for nv/logger/common.h
// Provides only the EventStructItem and Event entries used by nsm_type_5.cpp.
#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace nv::logger {

constexpr auto EventDataSize = 8;
using EventData = std::array<uint8_t, EventDataSize>;
using EventId   = uint16_t;

enum class Level : uint8_t { Unknown, Debug, Info, Warning, Error, Critical };

struct EventStructItem {
    EventId unique_id;
    Level   default_level;
};

struct Event {
    static constexpr EventStructItem NsmLogMessages          = {0x0e00, Level::Info};
    static constexpr EventStructItem T5ActivateMcuException  = {0x0e01, Level::Info};
    static constexpr EventStructItem T5ActivateWatchdogTimeout = {0x0e02, Level::Info};
};

}  // namespace nv::logger
