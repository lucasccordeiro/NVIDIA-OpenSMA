// ESBMC harness for nv/soc_pwr_smoothing/soc_sma_filter_ch.h
//
// Function verified:
//   SocSmaFilterCh::evaluate(Ports&) — 4-sample sliding-window SMA over SFXP22_10.
//   Maintains a circular buffer of 4 int32_t samples; index advances with
//   bitwise-AND modulo; output = _sum >> 2.
//
// Phase 1: no arithmetic overflow or UB for soc_percent ∈ [0%, 100%].
// Phase 2: functional contracts —
//   - steady-state: starting from zero state, 4 equal inputs x produce
//     output == x on the 4th call (buffer fills with x; _sum == 4x; 4x >> 2 == x).
//   - output bounded: one evaluate() call from zero state with bounded input
//     produces output in [0, input].
//
// Include notes: soc_sma_filter_ch.h only depends on fixed_point.h and mpf.h;
// both are header-only with no MCU platform includes.

#include "nv/soc_pwr_smoothing/soc_sma_filter_ch.h"

using namespace nv::soc_pwr_smoothing;
using namespace nv;

extern "C" {
int32_t  nondet_i32();
unsigned nondet_uint();
}

namespace {

// SFXP22_10 encoding: 1 unit = 1/1024 %; 100% = 102400.
constexpr SFXP22_10 kMin = to_sfxp22_10(0);    // 0%
constexpr SFXP22_10 kMax = to_sfxp22_10(100);  // 100%

// ----- Phase 1: totality -----

// evaluate() once from zero state with any bounded input — no UB.
void f_totality()
{
    SocSmaFilterCh filter{};
    SFXP22_10      x = nondet_i32();
    __ESBMC_assume(x >= kMin && x <= kMax);

    SFXP22_10              out{};
    SocSmaFilterCh::Ports  ports{.soc_percent = x, .soc_percent_filtered = out};
    filter.evaluate(ports);
    (void)out;
}

// ----- Phase 2: functional contracts -----

// Starting from zero state, 4 calls with the same value x produce output == x.
//
// Derivation (buffer = {0,0,0,0}, _sum = 0):
//   call 1: _sum = x - 0 = x;  buf[0]=x;  out = x>>2
//   call 2: _sum = x + x - 0 = 2x; buf[1]=x; out = 2x>>2
//   call 3: _sum = 3x; buf[2]=x; out = 3x>>2
//   call 4: _sum = 4x; buf[3]=x; out = 4x>>2 = x  (integer shift, exact)
void f_steady_state()
{
    SocSmaFilterCh filter{};
    SFXP22_10      x = nondet_i32();
    __ESBMC_assume(x >= kMin && x <= kMax);

    SFXP22_10 out1{}, out2{}, out3{}, out4{};
    SocSmaFilterCh::Ports p1{.soc_percent = x, .soc_percent_filtered = out1};
    filter.evaluate(p1);
    SocSmaFilterCh::Ports p2{.soc_percent = x, .soc_percent_filtered = out2};
    filter.evaluate(p2);
    SocSmaFilterCh::Ports p3{.soc_percent = x, .soc_percent_filtered = out3};
    filter.evaluate(p3);
    SocSmaFilterCh::Ports p4{.soc_percent = x, .soc_percent_filtered = out4};
    filter.evaluate(p4);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(out4 == x,
                   "steady state: 4 equal inputs from zero state → output == input");
#endif
    (void)out1; (void)out2; (void)out3; (void)out4;
}

// One evaluate() from zero state with bounded input: output ∈ [0, input].
// After call 1: _sum = x; output = x >> 2 ∈ [0, x] for x ≥ 0.
void f_output_bounded()
{
    SocSmaFilterCh filter{};
    SFXP22_10      x = nondet_i32();
    __ESBMC_assume(x >= kMin && x <= kMax);

    SFXP22_10              out{};
    SocSmaFilterCh::Ports  ports{.soc_percent = x, .soc_percent_filtered = out};
    filter.evaluate(ports);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(out >= kMin,
                   "output non-negative after one bounded call from zero state");
    __ESBMC_assert(out <= x,
                   "output ≤ input after one bounded call from zero state");
#endif
    (void)out;
}

}  // namespace

int main()
{
    switch (nondet_uint() % 3) {
        case 0: f_totality();       break;
        case 1: f_steady_state();   break;
        case 2: f_output_bounded(); break;
    }
    return 0;
}
