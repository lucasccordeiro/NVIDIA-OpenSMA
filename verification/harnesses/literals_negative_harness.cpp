// Negative harness for the `_bit(i)` user-defined literal: `i >= 64`
// is undefined behaviour ([expr.shift]/1) for `unsigned long long << i`
// since the shift count exceeds the type width. ESBMC must report a
// counterexample under --ub-shift-check.

#include <cstdint>

extern "C" uint64_t nondet_u64();

constexpr auto bit(unsigned long long i)
{
    return static_cast<decltype(i)>(1) << i;
}

int main()
{
    uint64_t i = nondet_u64();
    __ESBMC_assume(i >= 64 && i < 128);  // out-of-range, deliberately
    return static_cast<int>(bit(i));
}
