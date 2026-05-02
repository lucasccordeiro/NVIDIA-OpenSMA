// Verification stub: sys/sensor/sensor.h
// Mirrors the x86 platform version (src/sys/x86/sys/sensor/sensor.h).
// Production code calls sys::sensor::Driver::get_current_temperature(); the stub
// returns false so callers take the "sensor unavailable" path.
#pragma once

namespace sys::sensor {

class Driver
{
public:
    static void init() {}
    static bool get_current_temperature(float& /*result*/) { return false; }
};

}  // namespace sys::sensor
