// ESBMC system-level harness for F-13: reachability proof via PowerManager::run_iteration().
//
// Finding: DebugTelemetrySmaCh::evaluate() (debug_telemetry_sma_ch.h:51) casts
//   SFXP32_0 percent to UFXP8_0 (uint8_t) without checking for negative values.
//   A negative percent wraps modulo 256, corrupting the SMA buffer.
//
// This harness calls the REAL PowerManager::run_iteration() (production code,
// not an inline model) with a nondet ADC reading — the only external hardware
// input in the loop.  GPIO (thermal warning) is also nondet so both thermal-
// brake asserted and deasserted paths are explored.
//
// Reachability argument:
//   soc_percent_filtered : StateOfChargeDev::soc_voltage_to_percent() applies
//     std::clamp(%, 0%, 100%) before the value enters the SMA.
//   edpp_offset          : OffsetPolicy::run_policy<Edpp> returns
//     std::clamp(critical+residency, 0%, 100%).  reset paths return 0%.
//   isink_offset         : OffsetPolicy::run_policy<Isink> returns
//     100% - std::clamp(...) ∈ [0%, 100%].  reset paths return 100% or 0%.
//   All three channels are ≥ 0 for every possible hardware input.
//
// Expected: VERIFICATION SUCCESSFUL — upstream clamping prevents the negative-
// wrap bug from being triggered.  F-13 is a latent defect: the function is
// unsafe when called with negative input but the current data flow prevents
// that from happening.

#include "nv/soc_pwr_smoothing/power_manager.h"

using namespace nv;
using namespace nv::soc_pwr_smoothing;

extern "C" {
uint16_t nondet_u16();
uint8_t  nondet_u8();
}

int main()
{
    PowerManager pm{};

    // One iteration: ADC reading and GPIO (thermal-warn) are both nondet,
    // covering the full hardware input space in a single bounded step.
    pm.run_iteration();

    // The three DebugTelemetrySmaCh percent_averaged outputs must be
    // non-negative.  Upstream clamping makes the negative-wrap unreachable.
    __ESBMC_assert(
        pm.public_connectors.soc_percent_avg  >= static_cast<SFXP22_10>(0)
     && pm.public_connectors.edpp_offset_avg  >= static_cast<SFXP22_10>(0)
     && pm.public_connectors.isink_offset_avg >= static_cast<SFXP22_10>(0),
        "F-13 system: DebugTelemetrySmaCh outputs non-negative for all hardware inputs");

    return 0;
}
