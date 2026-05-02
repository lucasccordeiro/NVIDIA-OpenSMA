// Verification stub for sys::gpio::Driver.
// read() returns nondet so ESBMC explores both GPIO states (e.g. thermal warning
// asserted or deasserted).  write() is a no-op.
#pragma once
#include <cstdint>

extern "C" uint8_t nondet_u8();

namespace sys::gpio {

class Driver
{
public:
    template<uint32_t /*port*/, uint32_t /*pin*/>
    static void write(uint8_t /*data*/) {}

    template<uint32_t /*port*/, uint32_t /*pin*/>
    static uint8_t read() { return nondet_u8(); }

protected:
    static void init() {}
};

}  // namespace sys::gpio
