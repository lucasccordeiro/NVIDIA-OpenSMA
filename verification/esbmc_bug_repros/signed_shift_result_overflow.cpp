// Minimal repro: signed left-shift result overflow is UB in C++20.
// Under C++17 [expr.shift]/2 and C++20 (C++23 made it defined via P0907R4
// follow-up), (int)128 << 24 = 2147483648 > INT_MAX -- UB.
//
// Expected with --std c++20 --overflow-check: VERIFICATION FAILED
// Actual: VERIFICATION SUCCESSFUL (ESBMC's bitvector model wraps silently)
#include <cstdint>
extern "C" { uint8_t nondet_u8(); }
int main() {
    uint8_t b = nondet_u8();
    __ESBMC_assume(b >= 128);          // forces the overflowing path
    volatile int r = (int)b << 24;    // UB: result > INT_MAX for b >= 128
    (void)r;
    return 0;
}
