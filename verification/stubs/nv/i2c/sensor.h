// Verification stub for nv/i2c/sensor.h
//
// The production header pulls in nv/telemetry/cache.h → nv/telemetry/utils.h
// (already verified), nv/mctp/enums.h (corepdk), and nv/i2c/i2c_types.h
// (sensor config structs with packed arrays of those types).  For
// emc1812 / tmp1075 verification we only need: I2cStatus, Port, TelemId,
// and a TempSensor base class whose I2C methods return nondet values.
#pragma once

#include <stdint.h>
#include <span>

#include "nv/i2c/port.h"

namespace nv::telemetry {
enum TelemId : uint8_t { MaxItem = 255 };
}

namespace nv::i2c {

enum class I2cStatus : uint8_t
{
    Ok,
    Error,
    Busy,
    Nak,
    Timeout,
    ArbLost,
    MutexError
};

extern "C" {
uint8_t  nondet_u8();
uint16_t nondet_u16();
uint8_t  nondet_i2c_status();
}

class TempSensor
{
public:
    TempSensor(Port, uint8_t, nv::telemetry::TelemId = nv::telemetry::TelemId::MaxItem) {}

    I2cStatus read_reg(uint8_t, uint8_t& value)
    {
        value = nondet_u8();
        auto s = static_cast<I2cStatus>(nondet_i2c_status() % 7);
        return s;
    }

    I2cStatus write_reg(uint8_t, uint8_t)
    {
        auto s = static_cast<I2cStatus>(nondet_i2c_status() % 7);
        return s;
    }

    I2cStatus read_reg_16bits(uint8_t, uint16_t& value)
    {
        value = nondet_u16();
        auto s = static_cast<I2cStatus>(nondet_i2c_status() % 7);
        return s;
    }

    I2cStatus write_reg_16bits(uint8_t, uint16_t)
    {
        auto s = static_cast<I2cStatus>(nondet_i2c_status() % 7);
        return s;
    }

    I2cStatus read_block(uint8_t, std::span<uint8_t>, uint8_t& length)
    {
        length = nondet_u8();
        auto s = static_cast<I2cStatus>(nondet_i2c_status() % 7);
        return s;
    }
};

}  // namespace nv::i2c
