// ESBMC verification stub for nv/i2c/error_injection.h
// Port comes from lattice_driver.h (enum class Port : uint8_t); we must NOT
// redefine it here.  All ports/functions are stubs returning
// NV_I2C_MAX_ERROR_INJECTION_PORTS (invalid).
#pragma once
#include <array>
#include <cstdint>
#include "nv/i2c/lattice_driver.h"
#include "nv/ipchandler/enums.h"
#include NV_IPC_CONFIG_H

namespace nv::i2c {

enum class ErrorInjectionType : uint8_t {
    Clear        = 0x00,
    QueueFull    = 0x01,
    Nack         = 0x02,
    Timeout      = 0x03,
    UsbQueueFull = 0x04,
};

enum class ProtocolType : uint8_t {
    I2c        = 0x02,
    I2cPca9555 = 0x05,
};

struct ErrorInjectionConfig {
    bool    enabled;
    uint8_t error_type;
    uint8_t target_address;
    uint8_t protocol_type;
};

struct ErrorInjectionPortMapping {
    nv::ipchandler::Id ipchandler_id;
    Port               port;
};

constexpr size_t MaxErrorInjectionPorts = NV_I2C_MAX_ERROR_INJECTION_PORTS;

inline const std::array<ErrorInjectionPortMapping, NV_I2C_MAX_ERROR_INJECTION_PORTS>
    ErrorInjectionPortMappingTable{};

inline constexpr size_t port_to_error_injection_index(Port, uint8_t)
{
    return NV_I2C_MAX_ERROR_INJECTION_PORTS;
}

inline void enable_error_injection(Port, ErrorInjectionType, uint8_t = 0x0,
                                   ProtocolType = ProtocolType::I2c) {}

}  // namespace nv::i2c
