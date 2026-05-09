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

    // SSIF events (referenced by src/nv/ssif/ssif.cpp).
    static constexpr EventStructItem SsifTxUnexpectedReadMulti  = {0x0e10, Level::Warning};
    static constexpr EventStructItem SsifTxUnexpectedCommand    = {0x0e11, Level::Warning};
    static constexpr EventStructItem SsifRxUnexpectedCmd        = {0x0e12, Level::Warning};
    static constexpr EventStructItem SsifRxInvalidSize          = {0x0e13, Level::Warning};
    static constexpr EventStructItem SsifRxInvalidPec           = {0x0e14, Level::Warning};
    static constexpr EventStructItem SsifRxUnexpectedWriteMulti = {0x0e15, Level::Warning};

    // I2C slave driver events (referenced by sys/i2c/i2c_slave.h template,
    // unreachable in the no-op stubbed driver but kept for future harnesses).
    static constexpr EventStructItem I2CSlaveDriverError        = {0x0e20, Level::Error};
    static constexpr EventStructItem I2CSlaveUnexpectedEvent    = {0x0e21, Level::Error};
    static constexpr EventStructItem I2CSlaveRecovery           = {0x0e22, Level::Info};
};

// Status returned by Logger::add_from_isr; the harness ignores it.
enum class Status : uint8_t { Ok, Error };

// OutputDirection placeholder (not referenced by ssif but kept for parity).
enum class OutputDirection : uint8_t { Both, Flash, Console };

}  // namespace nv::logger
