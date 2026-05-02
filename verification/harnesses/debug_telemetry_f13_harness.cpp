// ESBMC negative harness for F-13: DebugTelemetrySmaCh negative percent wraps to large uint8_t.
//
// Finding: DebugTelemetrySmaCh::evaluate() (debug_telemetry_sma_ch.h line 51):
//   const SFXP32_0 percent = sfxp22_10_to_sfxp32_0(ports.percent);
//   _buffer[_index]        = static_cast<UFXP8_0>(percent);   // UFXP8_0 = uint8_t
//
// sfxp22_10_to_sfxp32_0 strips fractional bits: result = ports.percent >> 10.
// When ports.percent (SFXP22_10 = int32_t) is negative, the result is a negative int32_t.
// Casting a negative SFXP32_0 to UFXP8_0 (uint8_t) wraps modulo 256.
// Example: percent = -1 (SFXP22_10 = -1024 raw) → sfxp32_0 = -1 → uint8_t = 255.
//
// The existing positive harness constrains percent ∈ [0%, 150%]. This negative harness
// constrains percent < 0 (raw value < 0 as SFXP22_10) to expose the wrap.
//
// Post-condition (expected to FAIL): stored buffer value is in [0, 150].
// Expected: VERIFICATION FAILED — stored value wraps to a large uint8_t (e.g., 255).

#include "nv/soc_pwr_smoothing/debug_telemetry_sma_ch.h"

using namespace nv::soc_pwr_smoothing;
using namespace nv;

extern "C" {
int32_t  nondet_i32();
}

int main()
{
    DebugTelemetrySmaCh filter{};

    // Negative percent: raw SFXP22_10 value < 0, meaning actual percentage < 0%.
    SFXP22_10 x = nondet_i32();
    __ESBMC_assume(x < static_cast<SFXP22_10>(0));

    SFXP22_10                   out{};
    DebugTelemetrySmaCh::Ports  ports{.percent = x, .percent_averaged = out};
    filter.evaluate(ports);

    // Retrieve the stored buffer value.
    // The first call stores at _buffer[0] (index starts at 0, then increments to 1).
    // We cannot access _buffer directly (private), but we can observe the SMA output:
    // After one call from zero state: _sum = 0 - _buffer[0] + percent = percent
    // (old _buffer[0] was 0). So percent_averaged = sfxp32_0_to_sfxp22_10(_sum) >> 8
    //                                              = (_sum << 10) >> 8 = _sum << 2
    // But _buffer[_index-1] = static_cast<uint8_t>(percent_integer).
    //
    // The stored value is cast to uint8_t inside evaluate(). We cannot directly read it,
    // but if the cast wraps, _sum will be corrupted because the next call subtracts the
    // wrapped value. Instead, we assert directly on the intermediate computation.

    // Inline the relevant computation to make the assertion checkable:
    const SFXP32_0 percent_int = sfxp22_10_to_sfxp32_0(x);  // x >> 10 (arithmetic shift)
    const UFXP8_0  stored      = static_cast<UFXP8_0>(percent_int);  // uint8_t cast

    // Post-condition: the stored buffer value should be in [0, 150] for a valid percentage.
    // For negative percent_int, the cast wraps to a large positive value, violating this.
    __ESBMC_assert(
        stored <= static_cast<UFXP8_0>(150),
        "F-13: negative percent wraps to large uint8_t in SMA buffer (> 150)");

    return 0;
}
