// ESBMC harness for nv::fixed_point conversions (src/nv/common/fixed_point.h).
// Header-only, no project deps. Phase 1 covers overflow / shift UB; Phase 2
// covers round-trip equivalence.
#include <cstdint>

#include "nv/common/fixed_point.h"

extern "C" {
int32_t  nondet_s32();
uint32_t nondet_u32();
unsigned nondet_uint();
}

namespace {

void f_to_sfxp22_10_safe()
{
    int32_t v = nondet_s32();
    __ESBMC_assume(v >= (INT32_MIN >> 10));
    __ESBMC_assume(v <= (INT32_MAX >> 10));
    int32_t out = nv::sfxp32_0_to_sfxp22_10(v);
#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(nv::sfxp22_10_to_sfxp32_0(out) == v, "sfxp22_10 round-trip");
#else
    (void)out;
#endif
}

void f_to_sfxp32_0_total() { (void)nv::sfxp22_10_to_sfxp32_0(nondet_s32()); }
void f_sfxp1_31_total()    { (void)nv::sfxp1_31_to_sfxp22_10(nondet_u32()); }
void f_to_float_total()    { (void)nv::sfxp22_10_to_float(nondet_s32()); }

void f_multiply_in_range()
{
    int32_t a = nondet_s32(), b = nondet_s32();
    __ESBMC_assume(a > -46340 && a < 46340);
    __ESBMC_assume(b > -46340 && b < 46340);
    (void)nv::sfxp22_10_multiply(a, b);
}

}  // namespace

int main()
{
    switch (nondet_uint() % 5) {
        case 0: f_to_sfxp22_10_safe(); break;
        case 1: f_to_sfxp32_0_total(); break;
        case 2: f_sfxp1_31_total(); break;
        case 3: f_multiply_in_range(); break;
        case 4: f_to_float_total(); break;
    }
    return 0;
}
