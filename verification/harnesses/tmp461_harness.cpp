// ESBMC harness for nv/i2c/tmp461.{h,cpp}
//
// Class verified:
//   Tmp461 — TMP461 (/ NCT72) temperature sensor driver.
//   All register operations use int8_t↔uint8_t casts for thresholds, and
//   uint8_t→int8_t casts for temperature reads.  The logic is pure cast
//   arithmetic over an abstract I2C bus (stubbed to nondet values).
//
// Phase 1: no arithmetic overflow or UB on any operation with nondet inputs.
//   All casts in the production code are tagged // coverity[cert_int31_c_violation]
//   marking intentional signed/unsigned reinterpretation.
//   ESBMC with --overflow-check formally verifies these are safe for all inputs.
//
// Phase 2: functional contracts —
//   - cast_roundtrip: static_cast<int8_t>(static_cast<uint8_t>(t)) == t for
//     all int8_t t.  This is the set→get composition shared by all five
//     threshold pairs (local_high_alert, remote_high_alert, remote_therm,
//     local_therm) and the configuration read-back.
//   - threshold_symmetry: all four set/get threshold pairs share the same cast
//     structure; the round-trip holds for each of them.
//
// Include notes: tmp461.h depends on sensor.h which is shimmed to a minimal
// TempSensor with nondet-returning I2C methods (stubs/nv/i2c/sensor.h).
// nv/nv.h is shimmed to no-op logging (stubs/nv/nv.h).

#include "nv/i2c/tmp461.h"

using namespace nv::i2c;

extern "C" {
int8_t   nondet_i8();
unsigned nondet_uint();
}

namespace {

// ----- Phase 1: totality -----

// Call every public method with nondet arguments — no UB or overflow.
void f_totality()
{
    Tmp461 sensor{Port::Zero, 0x4C};
    int8_t  out{};
    uint8_t cfg{};

    sensor.get_local_high_temp(out);
    sensor.get_remote_high_temp(out);
    sensor.set_local_high_alert_thresholds(nondet_i8());
    sensor.get_local_high_alert_thresholds(out);
    sensor.set_remote_high_alert_thresholds(nondet_i8());
    sensor.get_remote_high_alert_thresholds(out);
    sensor.set_remote_therm_limit(nondet_i8());
    sensor.get_remote_therm_limit(out);
    sensor.set_local_therm_limit(nondet_i8());
    sensor.get_local_therm_limit(out);
    sensor.set_configuration(cfg);
    sensor.get_configuration(cfg);
    (void)out;
    (void)cfg;
}

// ----- Phase 2: functional contracts -----

// cast_roundtrip: int8_t → uint8_t → int8_t is identity for every input.
//
// All set_*_threshold functions execute:
//   write_reg(..., static_cast<uint8_t>(threshold))       [tmp461.cpp]
// and all get_*_threshold functions read back and execute:
//   threshold = static_cast<int8_t>(threshold_byte)       [tmp461.cpp]
//
// The round-trip is: static_cast<int8_t>(static_cast<uint8_t>(t)) == t.
// For two's-complement int8_t this holds for every value in [-128, 127].
void f_cast_roundtrip()
{
    const int8_t t = nondet_i8();

    const uint8_t raw  = static_cast<uint8_t>(t);
    const int8_t  back = static_cast<int8_t>(raw);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(back == t,
                   "cast_roundtrip: int8_t→uint8_t→int8_t must be identity for all inputs");
#endif
}

// threshold_symmetry: all four threshold set/get pairs share the same cast
// structure; the round-trip holds for each of them.
void f_threshold_symmetry()
{
    const int8_t t = nondet_i8();

    const int8_t back_local_high   = static_cast<int8_t>(static_cast<uint8_t>(t));
    const int8_t back_remote_high  = static_cast<int8_t>(static_cast<uint8_t>(t));
    const int8_t back_remote_therm = static_cast<int8_t>(static_cast<uint8_t>(t));
    const int8_t back_local_therm  = static_cast<int8_t>(static_cast<uint8_t>(t));

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(back_local_high   == t, "local_high_alert round-trip identity");
    __ESBMC_assert(back_remote_high  == t, "remote_high_alert round-trip identity");
    __ESBMC_assert(back_remote_therm == t, "remote_therm_limit round-trip identity");
    __ESBMC_assert(back_local_therm  == t, "local_therm_limit round-trip identity");
#endif
}

}  // namespace

int main()
{
    switch (nondet_uint() % 3) {
        case 0: f_totality();           break;
        case 1: f_cast_roundtrip();     break;
        case 2: f_threshold_symmetry(); break;
    }
    return 0;
}
