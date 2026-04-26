// ESBMC 8.2.0 libc++ shim: <bit> is not bundled (no std::bit_cast,
// std::has_single_bit, std::popcount, etc.).
// Reproduce: esbmc --std c++20 stdlib_gap_bit.cpp
#include <bit>
int main()
{
    int v = 1;
    return std::has_single_bit((unsigned)v) ? 0 : 1;
}
