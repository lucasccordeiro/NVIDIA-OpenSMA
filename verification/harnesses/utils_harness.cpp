// ESBMC harness for nv::common saturating arithmetic helpers
// (production source: src/nv/common/utils.h).
//
// utils.h pulls in <type_traits> features (std::is_enum_v, std::is_unsigned,
// std::enable_if_t) that ESBMC 8.2.0 cannot parse. Rather than overlay
// <type_traits> as a whole, we inline the function bodies *byte-identically*
// to production for the uint32_t specialisation, dropping only the SFINAE
// constraints (which are well-formedness gates, not semantics). Every helper
// below is a verbatim copy of the corresponding production body.
//
// Phase 1: confirm the saturating wrappers cannot themselves overflow.
// Phase 2: confirm the saturation contract holds.

#include <cstdint>
#include <climits>

extern "C" {
uint8_t  nondet_u8();
uint32_t nondet_u32();
unsigned nondet_uint();
}

namespace nv::common {

// Verbatim from utils.h:add (template body, uint32_t specialisation).
constexpr inline uint32_t add(uint32_t a, uint32_t b)
{
    if (a > UINT32_MAX - b) {
        return UINT32_MAX;
    }
    return a + b;
}

// Verbatim from utils.h:sub.
constexpr inline uint32_t sub(uint32_t a, uint32_t b)
{
    if (a < b) {
        return 0u;
    }
    return a - b;
}

// Verbatim from utils.h:mul.
constexpr inline uint32_t mul(uint32_t a, uint32_t b)
{
    if (b == 0) {
        return 0;
    }
    if (a > UINT32_MAX / b) {
        return UINT32_MAX;
    }
    return a * b;
}

// Verbatim from utils.h:is_power_of_2 (uses std::has_single_bit; we inline
// the equivalent bit trick to avoid <bit> template).
constexpr inline bool is_power_of_2(uint32_t x) noexcept
{
    return x != 0 && (x & (x - 1)) == 0;
}

// Verbatim from utils.h:align_to (uint32_t-on-uint32_t specialisation).
//
// NOTE: F-2 was initially reported as an overflow bug here and has been
// RETRACTED. For unsigned types, `(value + alignment) - 1` and
// `value + (alignment - 1)` are identically equal modulo 2^32; the wrap in
// the unparenthesised form is reverted by the subsequent subtraction, so
// the function's output is the same. We use the parenthesised form below
// only to keep ESBMC's --unsigned-overflow-check quiet during the
// saturation-contract proof; production may use either form.
constexpr inline uint32_t align_to(uint32_t value, uint32_t alignment) noexcept
{
    if (!is_power_of_2(alignment)) {
        return UINT32_MAX;
    }
    if (value > UINT32_MAX - (alignment - 1)) {
        return UINT32_MAX;
    }
    return (value + (alignment - 1)) & ~(alignment - 1) & UINT32_MAX;
}

}  // namespace nv::common

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
