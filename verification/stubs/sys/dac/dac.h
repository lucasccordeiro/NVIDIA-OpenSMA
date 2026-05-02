// Verification stub for sys::dac::Dac.
// set() is a no-op; constants match the hardware platform values.
#pragma once
#include <cstdint>

namespace sys::dac {

class Dac
{
public:
    using Peripheral = uint32_t;

    static constexpr uint32_t ResolutionBits = 12;
    static constexpr uint32_t MaxVoltage_mV  = 3300;

    static void set(uint32_t /*value*/, Peripheral /*peripheral*/) {}

    Dac() = delete;
};

}  // namespace sys::dac
