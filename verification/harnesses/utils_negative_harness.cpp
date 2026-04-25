// Demonstrator: ESBMC's `--unsigned-overflow-check` on the unparenthesised
// `value + alignment - 1` form in nv::common::align_to.
//
// IMPORTANT: this is NOT a regression sentinel for a real bug. F-2 was
// retracted on review: in unsigned arithmetic
// `(value + alignment) - 1 ≡ value + (alignment - 1) (mod 2^N)`, so the
// wrap below is benign — it is reverted by the subsequent subtraction and
// the final masked result is correct.
//
// The harness is retained because:
//   (a) it documents what triggered the original (incorrect) F-2 claim, and
//   (b) it is a tidy reference for "ESBMC flags a wrap that doesn't matter
//       for the function's output" — the kind of false-positive shape future
//       harness authors should be ready to recognise.

#include <cstdint>
#include <climits>

extern "C" {
uint32_t nondet_u32();
}

namespace {

// Verbatim production semantics — the unparenthesised form that overflows.
constexpr inline bool is_power_of_2(uint32_t x) noexcept
{
    return x != 0 && (x & (x - 1)) == 0;
}

constexpr inline uint32_t align_to_buggy(uint32_t value, uint32_t alignment) noexcept
{
    if (!is_power_of_2(alignment)) {
        return UINT32_MAX;
    }
    if (value > UINT32_MAX - (alignment - 1)) {
        return UINT32_MAX;
    }
    // Production form: left-to-right associativity makes this
    //   (value + alignment) - 1
    // which overflows for value = alignment = 2^31 even though the guard
    // above passes (because the guard parenthesises differently).
    return (value + alignment - 1) & ~(alignment - 1) & UINT32_MAX;
}

}  // namespace

int main()
{
    uint32_t v = nondet_u32();
    uint32_t A = nondet_u32();
    (void)align_to_buggy(v, A);  // expected counterexample at v=A=0x80000000
    return 0;
}
