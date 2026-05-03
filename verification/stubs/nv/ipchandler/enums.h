// ESBMC verification stub for nv/ipchandler/enums.h
#pragma once
#include <cstdint>

namespace nv::ipchandler {

enum class Id : uint8_t {
    Mctp = 0,
    I2c0, I2c1, I2c2, I2c3, I2c4, I2c5, I2c6, I2c7, I2c8, I2c9,
    I3cStart, I3c0, I3c1, I3cEnd,
    Pldm, Usb, Flash, Logger, Spdm, Iox, Lstp, Unuse,
};

}  // namespace nv::ipchandler
