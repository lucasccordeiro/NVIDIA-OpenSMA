// F-13 failure-mode demonstrator: DebugTelemetrySmaCh negative percent wraps to large uint8_t.
//
// Production: DebugTelemetrySmaCh::evaluate() (debug_telemetry_sma_ch.h line 51):
//   const SFXP32_0 percent_int = sfxp22_10_to_sfxp32_0(ports.percent);  // x >> 10
//   _buffer[_index]            = static_cast<UFXP8_0>(percent_int);      // uint8_t cast
//
// SFXP22_10 = int32_t with 10 fractional bits. sfxp22_10_to_sfxp32_0 strips
// the fractional part via arithmetic right-shift by 10. When ports.percent is
// negative, percent_int is a negative int32_t, and the uint8_t cast wraps
// modulo 256. Example: percent = -1024 (= -1%) → sfxp32_0 = -1 → uint8_t = 255.
// The SMA buffer now contains 255 instead of a value in [0, 150], corrupting
// the running sum and all future averaged outputs.
//
// Expected runtime behaviour: assert fires because stored > 150.

#include <cassert>
#include <cstdint>

extern "C" {
int  __VERIFIER_nondet_int(void);
void __VERIFIER_assume(int cond);
}

using SFXP22_10 = int32_t;
using SFXP32_0  = int32_t;
using UFXP8_0   = uint8_t;

static inline SFXP32_0 sfxp22_10_to_sfxp32_0(SFXP22_10 x)
{
    return x >> 10;  // arithmetic right-shift strips 10 fractional bits
}

int main()
{
    SFXP22_10 x = __VERIFIER_nondet_int();
    __VERIFIER_assume(x < 0);  // negative percentage

    const SFXP32_0 percent_int = sfxp22_10_to_sfxp32_0(x);
    const UFXP8_0  stored      = static_cast<UFXP8_0>(percent_int);

    // Post-condition: SMA buffer slot must hold a value in [0, 150].
    assert(stored <= static_cast<UFXP8_0>(150) &&
           "F-13: negative percent wraps to large uint8_t in SMA buffer");
    return 0;
}
