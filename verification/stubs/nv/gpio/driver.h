// ESBMC verification stub for nv/gpio/driver.h
// Provides nv::gpio::Driver with no-op static methods. read() returns a nondet
// byte so both GPIO states (high/low) are explored by the model checker.
// Types (GpioPin, GpioPort, Direction, Status, etc.) come from nv/gpio/common.h
// to avoid redefinition conflicts with the real header.
#pragma once
#include <cstdint>
#include "nv/gpio/common.h"

extern "C" uint8_t nondet_u8();

namespace nv::gpio {

class Driver {
public:
    static void   init() {}
    static Status init_nonpriv_access(GpioPort, GpioPin) { return Status::Ok; }
    static Status init_pin(GpioPort, GpioPin, Direction, GpioState) { return Status::Ok; }
    static Status init_pin_cfg(GpioPort, GpioPin, GpioPullDir, GpioPullStrength, GpioOpenDrain)
        { return Status::Ok; }
    static Status init_interrupt(GpioPort, GpioPin, InterruptDetection, InterruptSelect)
        { return Status::Ok; }
    static Status init_interrupt_impl(GpioPort, GpioPin, InterruptDetection, InterruptSelect)
        { return Status::Ok; }
    static Status init_interrupt_svc(GpioPort, GpioPin, InterruptDetection, InterruptSelect)
        { return Status::Ok; }
    static Status read(GpioPort, GpioPin, uint8_t& data) {
        data = nondet_u8();
        return Status::Ok;
    }
    static Status read_virtual_physical_gpio(GpioPort, GpioPin, uint8_t& data) {
        data = nondet_u8();
        return Status::Ok;
    }
    static void   push_virtual_gpio_level(uint16_t, uint8_t) {}
    static Status read_gpio_port(GpioPort, uint32_t& bm) { bm = 0; return Status::Ok; }
    static Status write(GpioPort, GpioPin, uint8_t) { return Status::Ok; }
    static Status getDirection(GpioPort, GpioPin, Direction& dir) {
        dir = Direction::Input;
        return Status::Ok;
    }
};

}  // namespace nv::gpio
