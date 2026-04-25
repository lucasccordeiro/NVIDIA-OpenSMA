// F-2 retraction validator — empirical check.
//
// If the unsigned wrap in production's align_to is truly benign, then
// `align_buggy(v, A) == align_fixed(v, A)` for ALL inputs (the wrap in
// `(v + A) - 1` is reverted exactly by modular arithmetic before the mask
// is applied). ESBMC's branch-coverage ctest generator will produce
// concrete-value test cases; if every generated test passes the equality
// assertion, the retraction is empirically grounded.

#include <cstdint>
#include <climits>

// The ESBMC ctest scaffolding force-includes esbmc_verifier.h which
// declares __VERIFIER_nondet_uint() returning unsigned int.

constexpr inline bool is_pow2(uint32_t x)
{
    return x != 0 && (x & (x - 1)) == 0;
}

// Production form (the one I initially called "buggy" in F-2).
constexpr inline uint32_t align_buggy(uint32_t v, uint32_t A)
{
    if (!is_pow2(A))                          return UINT32_MAX;
    if (v > UINT32_MAX - (A - 1))             return UINT32_MAX;
    return (v + A - 1) & ~(A - 1) & UINT32_MAX;   // unparenthesised
}

// Parenthesised form (my proposed "fix").
constexpr inline uint32_t align_fixed(uint32_t v, uint32_t A)
{
    if (!is_pow2(A))                          return UINT32_MAX;
    if (v > UINT32_MAX - (A - 1))             return UINT32_MAX;
    return (v + (A - 1)) & ~(A - 1) & UINT32_MAX; // parenthesised
}

int main()
{
    uint32_t v = __VERIFIER_nondet_uint();
    uint32_t A = __VERIFIER_nondet_uint();

    uint32_t r_buggy = align_buggy(v, A);
    uint32_t r_fixed = align_fixed(v, A);

    // Empirical equivalence claim.
    if (r_buggy != r_fixed) {
        return 1;  // would indicate F-2 is real after all
    }
    return 0;
}
