// F-3 candidate validator: are the production and parenthesised forms of
// buf_to_u32 byte-equal across all uint32_t values + start indices?
//
// If yes, the production form's signed shift is benign under C++20+ wrap
// rules (same retraction shape as F-2). If no, F-3 is real.
#include <cstdint>

extern "C" unsigned int __VERIFIER_nondet_uint();
extern "C" unsigned char __VERIFIER_nondet_uchar();

constexpr uint8_t ByteShift1 = 8;
constexpr uint8_t ByteShift2 = 16;
constexpr uint8_t ByteShift3 = 24;

// Production form (cast outside the shift).
uint32_t prod(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
    return (static_cast<uint32_t>(b0 << ByteShift3))
         | (static_cast<uint32_t>(b1 << ByteShift2))
         | (static_cast<uint32_t>(b2 << ByteShift1))
         | (static_cast<uint32_t>(b3));
}

// Parenthesised form (cast inside, no signed shift).
uint32_t fixed(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
    return (static_cast<uint32_t>(b0) << ByteShift3)
         | (static_cast<uint32_t>(b1) << ByteShift2)
         | (static_cast<uint32_t>(b2) << ByteShift1)
         | (static_cast<uint32_t>(b3));
}

int main()
{
    uint8_t b0 = __VERIFIER_nondet_uchar();
    uint8_t b1 = __VERIFIER_nondet_uchar();
    uint8_t b2 = __VERIFIER_nondet_uchar();
    uint8_t b3 = __VERIFIER_nondet_uchar();
    return prod(b0, b1, b2, b3) == fixed(b0, b1, b2, b3) ? 0 : 1;
}
