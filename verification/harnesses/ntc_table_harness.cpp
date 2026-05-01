// ESBMC harness for src/nv/volt_mon/ntc_table.{h,cpp}
//
// Functions verified:
//   ntc_resistance_to_temperature: binary search (≤8 iters) + linear interpolation
//   ntc_voltage_to_temperature: voltage → resistance → temperature
//   ntc_adc_to_temperature: ADC reading → voltage → temperature
//   ntc_temperature_to_resistance: direct table lookup for t ∈ [−40, 125]
//   ntc_temp_to_adc_value: constexpr ADC encoder (table + arithmetic)
//
// Phase 1: all five functions terminate without UB (div-by-zero, overflow,
//          bounds violation) on any 16-/32-bit input.
// Phase 2: functional contracts —
//   - ntc_temperature_to_resistance: exact table value for valid t
//   - ntc_resistance_to_temperature: result always in [−400, 1250] (never NtcTempInvalid)
//   - round-trip: ntc_resistance_to_temperature(ntc_temperature_to_resistance(t))
//                 == t * NtcTempScale for all t ∈ [NtcTempMin, NtcTempMax]
//
// Binary search on 166 entries converges in ≤8 iterations; use --unwind 9.

#include <cstdint>
#include <climits>

#include "nv/volt_mon/ntc_table.h"

extern "C" {
uint16_t nondet_u16();
uint32_t nondet_u32();
int16_t  nondet_i16();
unsigned nondet_uint();
}

using namespace nv::volt_mon;

namespace {

// ----- Phase 1: totality — no UB on any input -----

void f_resistance_to_temp()
{
    uint32_t r = nondet_u32();
    (void)ntc_resistance_to_temperature(r);
}

void f_voltage_to_temp()
{
    uint16_t v = nondet_u16();
    (void)ntc_voltage_to_temperature(v);
}

void f_adc_to_temp()
{
    uint16_t adc  = nondet_u16();
    uint16_t vref = nondet_u16();
    (void)ntc_adc_to_temperature(adc, vref);
}

void f_temp_to_resistance()
{
    int16_t t = nondet_i16();
    (void)ntc_temperature_to_resistance(t);
}

void f_temp_to_adc()
{
    int16_t t = nondet_i16();
    (void)ntc_temp_to_adc_value(t);
}

// ----- Phase 2: functional contracts -----

void f_temp_to_resistance_contract()
{
    int16_t t = nondet_i16();
    __ESBMC_assume(t >= NtcTempMin && t <= NtcTempMax);
    uint32_t result = ntc_temperature_to_resistance(t);

#ifdef ESBMC_FUNCTIONAL
    const size_t  index    = static_cast<size_t>(t - NtcTempMin);
    const uint32_t expected = static_cast<uint32_t>(NtcResistanceTable[index])
                            * NtcResistanceScale;
    __ESBMC_assert(result == expected,
                   "ntc_temperature_to_resistance: exact table lookup for valid t");
#endif
    (void)result;
}

void f_resistance_to_temp_range()
{
    uint32_t r = nondet_u32();
    int16_t  result = ntc_resistance_to_temperature(r);

#ifdef ESBMC_FUNCTIONAL
    constexpr int16_t kMin = NtcTempMin * NtcTempScale;  // −400
    constexpr int16_t kMax = NtcTempMax * NtcTempScale;  // 1250
    __ESBMC_assert(result >= kMin && result <= kMax,
                   "ntc_resistance_to_temperature: result clamped to [−400, 1250]");
#endif
    (void)result;
}

void f_round_trip()
{
    int16_t t = nondet_i16();
    __ESBMC_assume(t >= NtcTempMin && t <= NtcTempMax);

    const uint32_t r  = ntc_temperature_to_resistance(t);
    const int16_t  t2 = ntc_resistance_to_temperature(r);

#ifdef ESBMC_FUNCTIONAL
    // The table stores exact resistance for each integer °C; the binary search
    // lands on the exact entry and interpolation ratio is 0, so the round-trip
    // recovers exactly t * NtcTempScale.
    __ESBMC_assert(t2 == static_cast<int16_t>(t * NtcTempScale),
                   "round-trip exact for all table-resident temperatures");
#endif
    (void)t2;
}

}  // namespace

int main()
{
    switch (nondet_uint() % 8) {
        case 0: f_resistance_to_temp();          break;
        case 1: f_voltage_to_temp();             break;
        case 2: f_adc_to_temp();                 break;
        case 3: f_temp_to_resistance();          break;
        case 4: f_temp_to_adc();                 break;
        case 5: f_temp_to_resistance_contract(); break;
        case 6: f_resistance_to_temp_range();    break;
        case 7: f_round_trip();                  break;
    }
    return 0;
}
