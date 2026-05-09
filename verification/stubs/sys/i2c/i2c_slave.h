// Verification stub for sys/i2c/i2c_slave.h
//
// The production header pulls in NV_IPC_CONFIG_H (platform-specific) plus
// the NXP MCUXpresso fsl_lpi2c.h SDK and a templated I2CSlaveDriver<T> wired
// through HW registers and FreeRTOS callbacks. None of that is reachable in
// verification — pca9555 only needs the buffer typedef; ssif additionally
// needs NumI2cTargetAddresses and a no-op I2CSlaveDriver<T>.
#pragma once

#include <array>
#include <stddef.h>
#include <stdint.h>

#include "nv/i2c/port.h"

#ifndef NUM_I2C_TARGET_ADDRESSES
#define NUM_I2C_TARGET_ADDRESSES 2
#endif

namespace sys::i2c {

constexpr static size_t I2cSlaveBufferSize    = 35;
constexpr static size_t NumI2cTargetAddresses = static_cast<size_t>(NUM_I2C_TARGET_ADDRESSES);

using I2cSlaveBuffer = std::array<uint8_t, I2cSlaveBufferSize>;

// Templated driver with no-op bind/start; the harness drives the public
// callbacks (i2c_callback / i2c_ack_callback) on the parent T directly,
// so the template's internal HW/IRQ machinery is not exercised here.
template<typename T>
class I2CSlaveDriver
{
public:
    I2CSlaveDriver() = default;

    void bind(nv::i2c::Port,
              std::array<uint8_t, NumI2cTargetAddresses> /*targets*/,
              T* /*parent*/)
    {
    }

    void start() {}
    void peripheral_recovery() {}
};

}  // namespace sys::i2c
