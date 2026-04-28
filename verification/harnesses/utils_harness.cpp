// ESBMC harness for nv::common saturating arithmetic helpers
// (production source: src/nv/common/utils.h).
//
// Phase 1: confirm the saturating wrappers cannot themselves overflow.
// Phase 2: confirm the saturation contract holds.

#include <cstdint>
#include "nv/common/utils.h"

extern "C" {
uint8_t  nondet_u8();
uint32_t nondet_u32();
unsigned nondet_uint();
}

using nv::common::add;
using nv::common::sub;
using nv::common::mul;
using nv::common::align_to;
using nv::common::is_power_of_2;

namespace {

void f_add_total()
{
    uint32_t a = nondet_u32(), b = nondet_u32();
    uint32_t r = add(a, b);
#ifdef ESBMC_FUNCTIONAL
    uint64_t natural = (uint64_t)a + (uint64_t)b;
    if (natural <= UINT32_MAX) {
        __ESBMC_assert(r == (uint32_t)natural, "add: in-range exact");
    } else {
        __ESBMC_assert(r == UINT32_MAX, "add: saturates to max");
    }
#else
    (void)r;
#endif
}

void f_sub_total()
{
    uint32_t a = nondet_u32(), b = nondet_u32();
    uint32_t r = sub(a, b);
#ifdef ESBMC_FUNCTIONAL
    if (a >= b) {
        __ESBMC_assert(r == a - b, "sub: in-range exact");
    } else {
        __ESBMC_assert(r == 0, "sub: saturates to 0");
    }
#else
    (void)r;
#endif
}

void f_mul_total()
{
    uint32_t a = nondet_u32(), b = nondet_u32();
    uint32_t r = mul(a, b);
#ifdef ESBMC_FUNCTIONAL
    uint64_t natural = (uint64_t)a * (uint64_t)b;
    if (natural <= UINT32_MAX) {
        __ESBMC_assert(r == (uint32_t)natural, "mul: in-range exact");
    } else {
        __ESBMC_assert(r == UINT32_MAX, "mul: saturates to max");
    }
#else
    (void)r;
#endif
}

void f_align_to_total()
{
    uint32_t v = nondet_u32();
    uint32_t A = nondet_u32();
    uint32_t r = align_to(v, A);
#ifdef ESBMC_FUNCTIONAL
    if (!is_power_of_2(A)) {
        __ESBMC_assert(r == UINT32_MAX, "align_to: non-pow2 -> max");
    } else if (v > UINT32_MAX - (A - 1)) {
        __ESBMC_assert(r == UINT32_MAX, "align_to: overflow -> max");
    } else {
        __ESBMC_assert((r & (A - 1)) == 0, "align_to: result is aligned");
        __ESBMC_assert(r >= v, "align_to: result >= input");
        __ESBMC_assert(r - v < A, "align_to: result within one alignment");
    }
#else
    (void)r;
#endif
}

void f_is_power_of_2_total()
{
    uint32_t x = nondet_u32();
    (void)is_power_of_2(x);
}

}  // namespace

int main()
{
    switch (nondet_uint() % 5) {
        case 0: f_add_total(); break;
        case 1: f_sub_total(); break;
        case 2: f_mul_total(); break;
        case 3: f_align_to_total(); break;
        case 4: f_is_power_of_2_total(); break;
    }
    return 0;
}
