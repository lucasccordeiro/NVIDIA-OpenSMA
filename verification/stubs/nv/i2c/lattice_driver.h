// ESBMC verification stub for nv/i2c/lattice_driver.h
// LatticeCpld methods return I2cStatus::Ok; inst() returns a static singleton.
#pragma once
#include <cstdint>
#include <span>
#include "nv/i2c/dummy_cpld_registers.h"

namespace nv::i2c {

constexpr size_t LATTICE_CPLD_FEATURE_ROW_SIZE = 10;

enum class Port : uint8_t { Begin, Zero = Begin, One, Two, Three, Four, Five,
                             Six, Seven, Eight, Nine, End };

enum class I2cStatus : uint8_t { Ok, Error, Busy, Nak, Timeout, ArbLost, MutexError };

class LatticeCpld {
public:
    static constexpr bool is_enabled() { return false; }
    LatticeCpld() noexcept = default;

    I2cStatus program_feature_row()                   { return I2cStatus::Ok; }
    I2cStatus read_feature_row(std::span<uint8_t>)    { return I2cStatus::Ok; }
    I2cStatus otp_feature_row()                       { return I2cStatus::Ok; }
    I2cStatus read_otp_feature_row(uint8_t& v)        { v = 0; return I2cStatus::Ok; }
    I2cStatus read_id()                               { return I2cStatus::Ok; }
    I2cStatus isc_enable()                            { return I2cStatus::Ok; }
    I2cStatus isc_disable()                           { return I2cStatus::Ok; }

    static LatticeCpld& inst() {
        static LatticeCpld c;
        return c;
    }
};

}  // namespace nv::i2c
