// ESBMC 8.2.0 (post-#4184) — spurious round-trip CEX through aliased
// `*std::bit_cast<T*>(...)`.
//
// `std::bit_cast<T*>(p)` returns a pointer with the same bit pattern
// as p. Dereferencing the result therefore aliases the same memory as
// dereferencing p. The property below should be trivially true by
// aliasing — yet ESBMC reports a CEX whose own state trace shows
// `mirror = &h` (i.e. the aliasing held), then nevertheless flags the
// equality as violated.
//
// Reproduce:
//   esbmc --std c++20 \
//         --overflow-check --interval-analysis \
//         --k-induction --k-step 1 --max-k-step 6 \
//         bit_cast_aliasing_cex.cpp
#include <bit>
#include <cstdint>
#include <span>

extern "C" uint16_t __VERIFIER_nondet_uint16_t();

struct [[gnu::packed]] Header
{
    uint16_t length;
    uint8_t  pad[6];
};

int main()
{
    Header h{};
    h.length = __VERIFIER_nondet_uint16_t();

    // Take a span over h, then recover h via bit_cast on the span data.
    std::span<uint8_t> s{(uint8_t*)&h, sizeof(h)};
    Header& mirror = *std::bit_cast<Header*>(s.data());

    // mirror and h alias the same memory; the equality is structural.
    __ESBMC_assert(mirror.length == h.length, "round-trip via bit_cast");
    return 0;
}
