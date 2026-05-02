// ESBMC harness for F-9: signed intermediate in buf_to_u32 / buf_to_u16.
//
// Finding: static_cast<uint32_t>(buf[start_idx] << ByteShift3)
//   buf[start_idx] is uint8_t, promoted to int before the shift.
//   Under C++20 [expr.shift]/2, left-shifting a non-negative int by N bits is defined
//   as long as the result fits in the value representation of the type.
//   For buf[0] >= 0x80 (i.e., 128), int is 32 bits, and 128 << 24 = 0x80000000 = INT_MIN,
//   which overflows signed 32-bit arithmetic.
//
// This harness runs with both --overflow-check and --ub-shift-check to check
// whether ESBMC fires on this expression.
//
// The harness inlines buf_to_u32 verbatim from src/nv/spi/utils.h.
//
// If neither flag fires on any path, the finding should be dropped as defined behaviour.
// If --overflow-check fires (signed overflow) or --ub-shift-check fires (UB shift),
// the finding is confirmed.
//
// Run 1: esbmc ... --overflow-check --unsigned-overflow-check
// Run 2: esbmc ... --overflow-check --unsigned-overflow-check --ub-shift-check

#include <cstdint>
#include <span>

extern "C" {
uint8_t nondet_u8();
}

// Constants verbatim from nv/spi/common.h
constexpr uint8_t ByteShift1 = 8;
constexpr uint8_t ByteShift2 = 16;
constexpr uint8_t ByteShift3 = 24;

// Verbatim from src/nv/spi/utils.h
inline uint32_t buf_to_u32(std::span<uint8_t> buf, uint8_t start_idx)
{
    if (start_idx + sizeof(uint32_t) > buf.size()) {
        return 0;
    }
    // buf[start_idx] is uint8_t → promoted to int → shifted left 24 bits.
    // For buf[start_idx] >= 0x80, (int)(0x80) << 24 = 0x80000000 which overflows int.
    return (static_cast<uint32_t>(buf[start_idx] << ByteShift3))
         | (static_cast<uint32_t>(buf[start_idx + 1] << ByteShift2))
         | (static_cast<uint32_t>(buf[start_idx + 2] << ByteShift1))
         | (static_cast<uint32_t>(buf[start_idx + 3]));
}

inline uint16_t buf_to_u16(std::span<uint8_t> buf, uint8_t start_idx)
{
    if (start_idx + sizeof(uint16_t) > buf.size()) {
        return 0;
    }
    // buf[start_idx] is uint8_t → promoted to int → shifted left 8 bits.
    // For buf[start_idx] >= 0x80, (int)(0x80) << 8 = 0x8000; fits in int, no overflow.
    return (static_cast<uint16_t>(buf[start_idx] << ByteShift1))
         | (static_cast<uint16_t>(buf[start_idx + 1]));
}

int main()
{
    // Provide a 4-byte buffer with nondet content.
    uint8_t storage[4] = {nondet_u8(), nondet_u8(), nondet_u8(), nondet_u8()};

    // Constrain buf[0] >= 0x80 to target the overflow-triggering case
    __ESBMC_assume(storage[0] >= 0x80u);

    std::span<uint8_t> buf{storage, 4};

    // Call with start_idx = 0 (in-bounds path guaranteed)
    uint32_t result32 = buf_to_u32(buf, 0);
    (void)result32;

    // Also exercise buf_to_u16 — shift is only 8 bits, less likely to overflow
    // but included for completeness
    uint8_t storage2[2] = {nondet_u8(), nondet_u8()};
    __ESBMC_assume(storage2[0] >= 0x80u);
    std::span<uint8_t> buf2{storage2, 2};
    uint16_t result16 = buf_to_u16(buf2, 0);
    (void)result16;

    return 0;
}
