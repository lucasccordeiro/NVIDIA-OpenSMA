// ESBMC harness for nv user-defined literal helpers
// (production source: src/nv/common/literals.h).
//
// Each operator is `constexpr auto operator""_<tag>(unsigned long long)`
// returning either a truncated integer or, for `_bit`, a left-shift.
// We inline the function bodies verbatim — production has no project
// dependencies beyond <cstdint>, but the user-defined-literal calling
// syntax is awkward to drive with nondet input, so the bodies live
// here under named entry points.
//
// Phase 1: totality + safety on the underlying casts/shifts.
// Phase 2 (-DESBMC_FUNCTIONAL=1):
//   - truncating casts agree with `n & mask` for unsigned, and with the
//     two's-complement modular result for signed (C++20 [conv.integral]/3);
//   - `_bits_sizeof(n) == n / 8`;
//   - `_bit(i)` agrees with `1ULL << i` for `i < 64`.

#include <cstdint>

extern "C" {
uint64_t nondet_u64();
unsigned nondet_uint();
}

namespace verif {

// Verbatim from src/nv/common/literals.h.
constexpr auto u8(unsigned long long n)
{
    return static_cast<uint8_t>(n);
}
constexpr auto u16(unsigned long long n)
{
    return static_cast<uint16_t>(n);
}
constexpr auto u32(unsigned long long n)
{
    return static_cast<uint32_t>(n);
}
constexpr auto i8(unsigned long long n)
{
    return static_cast<int8_t>(n);
}
constexpr auto i16(unsigned long long n)
{
    return static_cast<int16_t>(n);
}
constexpr auto i32(unsigned long long n)
{
    return static_cast<int32_t>(n);
}
constexpr auto bits_sizeof(unsigned long long bits)
{
    return bits / 8;
}
constexpr auto bit(unsigned long long i)
{
    return static_cast<decltype(i)>(1) << i;
}

}  // namespace verif

namespace {

void f_u8_total()
{
    uint64_t n = nondet_u64();
    auto     r = verif::u8(n);
#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(r == static_cast<uint8_t>(n & 0xFF), "u8 == n & 0xFF");
#else
    (void)r;
#endif
}

void f_u16_total()
{
    uint64_t n = nondet_u64();
    auto     r = verif::u16(n);
#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(r == static_cast<uint16_t>(n & 0xFFFFu), "u16 == n & 0xFFFF");
#else
    (void)r;
#endif
}

void f_u32_total()
{
    uint64_t n = nondet_u64();
    auto     r = verif::u32(n);
#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(r == static_cast<uint32_t>(n & 0xFFFFFFFFu),
                   "u32 == n & 0xFFFFFFFF");
#else
    (void)r;
#endif
}

void f_signed_truncations_total()
{
    // Signed truncating casts under C++20 are modular wraps. Phase 1
    // confirms no UB; the contract for the signed cast is "low byte/word
    // matches the unsigned mask", which we re-derive via the unsigned
    // companion to keep the assertion type-clean.
    uint64_t n = nondet_u64();

    auto i8v  = verif::i8(n);
    auto i16v = verif::i16(n);
    auto i32v = verif::i32(n);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(static_cast<uint8_t>(i8v) == verif::u8(n),
                   "i8 low byte matches u8");
    __ESBMC_assert(static_cast<uint16_t>(i16v) == verif::u16(n),
                   "i16 low word matches u16");
    __ESBMC_assert(static_cast<uint32_t>(i32v) == verif::u32(n),
                   "i32 low dword matches u32");
#else
    (void)i8v;
    (void)i16v;
    (void)i32v;
#endif
}

void f_bits_sizeof_total()
{
    uint64_t n = nondet_u64();
    auto     r = verif::bits_sizeof(n);
#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(r == n / 8, "bits_sizeof == n/8");
#else
    (void)r;
#endif
}

void f_bit_in_range()
{
    uint64_t i = nondet_u64();
    __ESBMC_assume(i < 64);  // the only defined range for unsigned long long shl
    auto r = verif::bit(i);
#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(r == (static_cast<uint64_t>(1) << i), "bit == 1ULL << i");
#else
    (void)r;
#endif
}

}  // namespace

int main()
{
    switch (nondet_uint() % 6) {
        case 0: f_u8_total(); break;
        case 1: f_u16_total(); break;
        case 2: f_u32_total(); break;
        case 3: f_signed_truncations_total(); break;
        case 4: f_bits_sizeof_total(); break;
        case 5: f_bit_in_range(); break;
    }
    return 0;
}
