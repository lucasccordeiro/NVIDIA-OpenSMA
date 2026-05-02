// F-10 failure-mode demonstrator: set_busbar_temperature_threshold silent 125°C substitution.
//
// Production: set_busbar_temperature_threshold (nsm_type_3.cpp lines 445-491):
//   tempCelsius = static_cast<int16_t>(request.threshold);  // uint8_t → int16_t
//   resistanceOhm = ntc_temperature_to_resistance(tempCelsius);
//   if (resistanceOhm == 0) {
//       // Invalid temperature — silently substitute 125°C (NtcTempMax)
//       resistanceOhm = ntc_temperature_to_resistance(NtcTempMax);
//       // falls through — no early return, no error code
//   }
//   ... return Ccode::Success;
//
// When threshold > 125 (> NtcTempMax), ntc_temperature_to_resistance returns 0
// (out-of-range). The caller receives Success instead of an error, and the MCU
// silently uses 125°C as the threshold.
//
// Expected runtime behaviour: assert fires because the function returns Success
// for an out-of-range threshold.

#include <cassert>
#include <cstdint>

extern "C" {
unsigned char __VERIFIER_nondet_uchar(void);
void          __VERIFIER_assume(int cond);
}

constexpr int16_t  NtcTempMin        = -40;
constexpr int16_t  NtcTempMax        = 125;
constexpr uint32_t NtcPullupResistor = 1000;
constexpr uint32_t NtcVref           = 3300;

enum class Ccode : uint8_t { Success = 0, ErrorInvalidData = 1, ErrorGeneral = 2 };

static uint32_t ntc_temperature_to_resistance(int16_t t)
{
    return (t < NtcTempMin || t > NtcTempMax) ? 0u : 1000u;
}

static Ccode set_busbar_temperature_threshold(uint8_t threshold_raw)
{
    const auto tempCelsius = static_cast<int16_t>(threshold_raw);
    uint32_t   resistanceOhm = ntc_temperature_to_resistance(tempCelsius);
    if (resistanceOhm == 0) {
        // BUG: silent substitution — should return an error code here.
        resistanceOhm = ntc_temperature_to_resistance(NtcTempMax);
    }
    const uint32_t voltageMv = NtcVref * resistanceOhm / (resistanceOhm + NtcPullupResistor);
    (void)voltageMv;
    return Ccode::Success;
}

int main()
{
    uint8_t threshold = __VERIFIER_nondet_uchar();
    __VERIFIER_assume(static_cast<int16_t>(threshold) > NtcTempMax);

    const Ccode result = set_busbar_temperature_threshold(threshold);

    assert(result != Ccode::Success &&
           "F-10: out-of-range threshold silently accepted, 125°C substituted");
    return 0;
}
