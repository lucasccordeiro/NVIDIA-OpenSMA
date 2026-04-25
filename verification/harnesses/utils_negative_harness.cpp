// Regression sentinel for finding F-2 (align_to overflow).
//
// This harness reproduces the production bug at src/nv/common/utils.h:82 by
// using the unparenthesised `value + alignment - 1` form. ESBMC must emit a
// counterexample on this harness; if it ever reports VERIFICATION SUCCESSFUL,
// either ESBMC's overflow check regressed or the input space was constrained
// — both are bugs.
//
// Once the production fix is applied (parenthesise as `value + (alignment - 1)`),
// this harness becomes obsolete and should be deleted along with F-2 itself.

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
