// ESBMC verification stub for nv/i2c/dummy_cpld_registers.h
// Provides the Cpld_User_Reg namespace with CPLD_USER_REG_SIZE, which is
// referenced by nsm_type_4.h for sizing segment_data arrays.
#pragma once
#include <cstdint>

namespace Cpld_User_Reg {
constexpr uint8_t CPLD_USER_REG_SIZE = 0x9C;
}  // namespace Cpld_User_Reg

#include <array>
namespace Cpld_Feature_Row {
constexpr std::array<uint8_t, 8> EXPECTED_FEATURE_ROW = {
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00};
constexpr std::array<uint8_t, 2> EXPECTED_FEABITS = {0x02, 0x00};
}  // namespace Cpld_Feature_Row
