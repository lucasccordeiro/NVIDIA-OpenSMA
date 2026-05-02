// Verification stub for sys/i2c/i2c_slave.h
//
// The production header pulls in NV_IPC_CONFIG_H (platform-specific).
// For verification we only need I2cSlaveBufferSize (= 35 on all platforms)
// and the I2cSlaveBuffer alias used by pca9555.h.
#pragma once

#include <array>
#include <stddef.h>
#include <stdint.h>

namespace sys::i2c {

constexpr static size_t I2cSlaveBufferSize = 35;

using I2cSlaveBuffer = std::array<uint8_t, I2cSlaveBufferSize>;

}  // namespace sys::i2c
