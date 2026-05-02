// ESBMC negative harness for F-10: set_busbar_temperature_threshold silently substitutes 125°C.
//
// Finding: set_busbar_temperature_threshold (nsm_type_3.cpp lines 445-491):
//   tempCelsius = static_cast<int16_t>(request.threshold);  // threshold is uint8_t
//   resistanceOhm = ntc_temperature_to_resistance(tempCelsius);
//   if (resistanceOhm == 0) {
//       // Invalid temperature, use default max temp (125°C)   <-- silent substitution
//       resistanceOhm = ntc_temperature_to_resistance(volt_mon::NtcTempMax);
//   }
//   ... continues to return Ccode::Success
//
// When ntc_temperature_to_resistance returns 0 (out-of-range temperature), the function
// substitutes 125°C silently and returns Success, instead of returning an error code.
//
// This harness directly compiles nv/volt_mon/ntc_table.cpp (production code) so ESBMC
// traces through the real 166-entry NTC lookup table rather than an inline model.
// The set_busbar_temperature_threshold logic is inlined verbatim (lines 445-491) with
// the hardware-specific BusbarTemp singleton stubbed to return Ok, matching F-1 rigor
// for the critical NTC path.
//
// Full compilation of nsm_type_3.cpp is blocked by esbmc#4245 (<optional>, <chrono>
// missing from ESBMC bundled library) and hardware headers (sys/adc/adc.h).
//
// Expected: VERIFICATION FAILED — the function returns Success even when the threshold
// causes ntc_temperature_to_resistance to return 0 (silent 125°C substitution).

#include <cstdint>

// --- Real production NTC lookup (src/nv/volt_mon/ntc_table.cpp) ---
// ntc_temperature_to_resistance() is declared in ntc_table.h; its implementation
// (ntc_table.cpp) compiles cleanly under ESBMC --std c++20.
#include "nv/volt_mon/ntc_table.h"

using namespace nv::volt_mon;

extern "C" {
uint8_t nondet_u8();
}

// Ccode verbatim from pdk-mctp-platforms-enums.h
enum class Ccode : uint8_t
{
    Success          = 0x00,
    ErrorGeneral     = 0x01,
    ErrorInvalidData = 0x02,
};

// Inline model of set_busbar_temperature_threshold (nsm_type_3.cpp lines 445-491).
// The BusbarTemp hardware singleton is stubbed: set_thresholds/get_sensor_info return Ok.
// The real ntc_temperature_to_resistance from ntc_table.cpp is called here.
static Ccode set_busbar_temperature_threshold(uint8_t threshold_raw)
{
    const auto tempCelsius = static_cast<int16_t>(threshold_raw);

    uint32_t resistanceOhm = ntc_temperature_to_resistance(tempCelsius);
    if (resistanceOhm == 0) {
        // BUG: silent substitution — should return error, but instead uses 125°C
        resistanceOhm = ntc_temperature_to_resistance(NtcTempMax);
        // falls through to return Success
    }

    const uint32_t voltageMv = NtcVref * resistanceOhm
                             / (resistanceOhm + NtcPullupResistor);
    (void)voltageMv;

    // busbarTemperature.set_thresholds() and get_sensor_info() stubbed to Ok
    // (hardware ADC interaction not relevant to the F-10 bug path)

    return Ccode::Success;
}

int main()
{
    uint8_t threshold = nondet_u8();

    // Target the out-of-range case: uint8_t values 126-255 cast to int16_t are
    // still positive but exceed NtcTempMax (125), so ntc_temperature_to_resistance returns 0.
    const auto tempCelsius = static_cast<int16_t>(threshold);
    __ESBMC_assume(tempCelsius > NtcTempMax || tempCelsius < NtcTempMin);

    // Confirm real production NTC lookup returns 0 on this path
    const uint32_t r = ntc_temperature_to_resistance(tempCelsius);
    __ESBMC_assume(r == 0);

    const Ccode result = set_busbar_temperature_threshold(threshold);

    __ESBMC_assert(
        result != Ccode::Success,
        "F-10: set_busbar_temperature_threshold must return error for out-of-range temperature");

    return 0;
}
