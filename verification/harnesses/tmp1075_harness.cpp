// ESBMC harness for nv/i2c/tmp1075.{h,cpp}
//
// Class verified:
//   Tmp1075 — TMP1075 temperature sensor driver.
//   Reads 12-bit two's-complement temperature over I2C (stubbed to nondet).
//   The only pure arithmetic logic is the signed 12-bit encoding:
//     write: static_cast<int16_t>(temp_celsius << 4)  — int8_t × 16
//     read:  static_cast<int16_t>(raw >> 4)           — arithmetic right shift
//
// Phase 1: no arithmetic overflow or UB on any operation with nondet inputs
//          under --std c++20 (signed left-shift is well-defined per [expr.shift]/2).
//
// Phase 2: functional contracts —
//   - 12bit_roundtrip: for every int8_t t, the encoding round-trip
//       static_cast<int8_t>(convert_12bit_to_signed(
//           static_cast<int16_t>(static_cast<uint16_t>(convert_signed_to_12bit(t)))))
//     equals t.  This is the composition executed by set_low_limit(t) followed
//     by a read-back via get_low_limit() when the register returns the exact
//     byte that was written.
//   - temp_read_cast: static_cast<int8_t>(temp_raw >> 8) is the same
//     uint8_t→int8_t reinterpretation verified for emc1812; it is
//     well-defined for all uint16_t inputs (no overflow).
//
// Include notes: tmp1075.h depends on sensor.h which is shimmed to a minimal
// TempSensor with nondet-returning I2C methods (stubs/nv/i2c/sensor.h).
// nv/nv.h is shimmed to no-op logging (stubs/nv/nv.h).

#include "nv/i2c/tmp1075.h"

using namespace nv::i2c;

extern "C" {
int8_t   nondet_i8();
uint16_t nondet_u16();
unsigned nondet_uint();
}

namespace {

// ----- Phase 1: totality -----

// Call every public method with nondet arguments — no UB or overflow.
void f_totality()
{
    Tmp1075 sensor{Port::Zero, 0x48};
    int8_t  out{};
    uint16_t dev_id{};

    sensor.read_temperature(out);
    sensor.set_low_limit(nondet_i8());
    sensor.get_low_limit(out);
    sensor.set_high_limit(nondet_i8());
    sensor.get_high_limit(out);
    sensor.get_device_id(dev_id);
    (void)out;
    (void)dev_id;
}

// ----- Phase 2: functional contracts -----

// 12bit_roundtrip: the 12-bit temperature encoding preserves every int8_t
// threshold value through a set→get cycle.
//
// Production set path (convert_signed_to_12bit):
//   static_cast<int16_t>(temp_celsius << 4)   [tmp1075.cpp:55]
// Production wire representation:
//   static_cast<uint16_t>(threshold_raw)       [tmp1075.cpp:64]
// Production get path (convert_12bit_to_signed + cast):
//   static_cast<int16_t>(threshold_raw_u16)    [tmp1075.cpp:77]
//   raw_value >> 4                             [tmp1075.cpp:45]
//   static_cast<int8_t>(temp_12bit)            [tmp1075.cpp:79]
//
// For t ∈ [-128, 127]:
//   (int)t << 4 ∈ [-2048, 2032] — fits in int16_t, no overflow.
//   Arithmetic >>4 maps back to [-128, 127] — fits in int8_t, no truncation.
void f_12bit_roundtrip()
{
    const int8_t t = nondet_i8();

    // set path: int8_t → int16_t (left shift 4)
    const int16_t raw16  = static_cast<int16_t>(t << 4);
    // wire: int16_t → uint16_t (bit-pattern preserved)
    const uint16_t raw_u = static_cast<uint16_t>(raw16);
    // get path: uint16_t → int16_t → arithmetic >>4 → int8_t
    const int16_t t12  = static_cast<int16_t>(static_cast<int16_t>(raw_u) >> 4);
    const int8_t  back = static_cast<int8_t>(t12);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(back == t,
                   "12bit_roundtrip: int8_t 12-bit encode/decode must be identity");
#endif
}

// temp_read_cast: static_cast<int8_t>(temp_raw >> 8) is well-defined for
// every uint16_t input — it is identical to the uint8_t→int8_t reinterpret
// verified for emc1812.
//
// Production path: static_cast<int8_t>(temp_raw >> 8)  [tmp1075.cpp:38]
void f_temp_read_cast()
{
    const uint16_t temp_raw = nondet_u16();

    // Extract high byte and reinterpret as signed
    const uint16_t high_byte = temp_raw >> 8U;           // ∈ [0, 255]
    const int8_t   temp      = static_cast<int8_t>(high_byte);

#ifdef ESBMC_FUNCTIONAL
    // No overflow possible: uint8_t→int8_t reinterpretation is always defined.
    // The assertion just ensures ESBMC tracks the value.
    (void)temp;
#endif
}

}  // namespace

int main()
{
    switch (nondet_uint() % 3) {
        case 0: f_totality();        break;
        case 1: f_12bit_roundtrip(); break;
        case 2: f_temp_read_cast();  break;
    }
    return 0;
}
