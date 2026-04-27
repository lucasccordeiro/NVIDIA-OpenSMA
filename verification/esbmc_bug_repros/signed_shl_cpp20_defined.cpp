// ESBMC 8.2.0 — `--overflow-check` flags a signed left-shift that is
// well-defined under C++20+ ([expr.shift]/2).
//
// Reproduce:
//   esbmc --std c++20 --overflow-check signed_shl_cpp20_defined.cpp
//
// Output:
//   Violated property:
//     arithmetic overflow on shl
//     !overflow("shl", (signed int)b, (signed int)24)
//   VERIFICATION FAILED
//
// Why the result is defined under C++20+:
//   [expr.shift]/2: "The value of E1 << E2 is the unique value congruent
//   to E1 × 2^E2 modulo 2^N, where N is the width of the result type."
//   For E1 = int(128), E2 = 24, N = 32: 128 × 2^24 = 2^31. The unique
//   int value congruent to 2^31 modulo 2^32 is INT_MIN. No UB.
//
// The cast to unsigned recovers the standard-defined bit pattern 0x80000000.

#include <cstdint>

extern "C" unsigned char __VERIFIER_nondet_uchar();

int main()
{
    uint8_t  b = __VERIFIER_nondet_uchar();
    uint32_t r = static_cast<uint32_t>(b << 24);  // defined under C++20+
    return r == 0 ? 0 : 1;
}
