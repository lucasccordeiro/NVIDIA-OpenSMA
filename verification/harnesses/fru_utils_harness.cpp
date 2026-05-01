// ESBMC harness for pure FRU utility functions from src/nv/fru/fru.cpp.
//
// Functions verified (inlined verbatim except range-for/std::fill replaced with
// index loops — functionally identical; required because the span stub lacks
// begin()/end() and std::array→span construction):
//   verify_checksum(span<const uint8_t>) — sum mod 256 == 0
//   decode_6bit_ascii(data, tlv_len, dst) — 6-bit packed ASCII → 8-bit ASCII
//
// Phase 1: both functions terminate without OOB or UB on any bounded input.
// Phase 2: functional contracts —
//   - verify_checksum: true iff (sum of all bytes) & 0xFF == 0
//   - verify_checksum({v}): true iff v == 0
//   - decode_6bit_ascii: each output byte in [0x20, 0x5F] when data sufficient

#include <cstdint>
#include <algorithm>
#include <cstring>

extern "C" {
uint8_t  nondet_u8();
unsigned nondet_uint();
}

namespace verif {

// ---- Constants (verbatim from fru.cpp) ----
constexpr uint8_t FRU_CHECKSUM_MASK         = 0xFF;
constexpr uint8_t SIXBIT_ASCII_SPACE_OFFSET = 0x20;
constexpr uint8_t SIXBIT_ASCII_MASK         = 0x3F;
constexpr uint8_t SIXBIT_CHARS_PER_GROUP    = 4;
constexpr uint8_t SIXBIT_CASE1_SHIFT        = 2;
constexpr uint8_t SIXBIT_CASE2_SHIFT        = 4;
constexpr uint8_t SIXBIT_CASE3_SHIFT        = 2;
constexpr uint8_t FRU_TLV_TYPE_SHIFT        = 6;

// ---- verify_checksum — verbatim logic, indexed loop replaces range-for ----
// Original uses: for (const uint8_t byte : data) { sum += byte; }
// The span stub lacks begin()/end(); semantics are identical.
bool verify_checksum(const uint8_t* data, unsigned len)
{
    uint32_t sum = 0;
    for (unsigned i = 0; i < len; i++) {
        sum += data[i];
    }
    return (sum & FRU_CHECKSUM_MASK) == 0;
}

// ---- decode_6bit_ascii — verbatim logic, memset replaces std::fill ----
bool decode_6bit_ascii(const uint8_t* data, uint8_t tlv_len, uint8_t* dst, uint16_t dst_len)
{
    for (uint16_t i = 0; i < dst_len; i++) {
        dst[i] = 0;
    }

    if (tlv_len == 0) {
        return false;
    }

    uint8_t i6 = 0;

    for (uint16_t i = 0; i < dst_len; i++) {
        const uint8_t byte_pos  = i % SIXBIT_CHARS_PER_GROUP;
        uint8_t       char_6bit = 0;

        switch (byte_pos) {
            case 0:
                if (i6 >= tlv_len) {
                    return false;
                }
                char_6bit = data[i6] & SIXBIT_ASCII_MASK;
                break;
            case 1:
                if (i6 + 1 >= tlv_len) {
                    return false;
                }
                char_6bit = (data[i6] >> FRU_TLV_TYPE_SHIFT)
                          | (data[i6 + 1] << SIXBIT_CASE1_SHIFT);
                i6++;
                break;
            case 2:
                if (i6 + 1 >= tlv_len) {
                    return false;
                }
                char_6bit = (data[i6] >> SIXBIT_CASE2_SHIFT)
                          | (data[i6 + 1] << SIXBIT_CASE2_SHIFT);
                i6++;
                break;
            case 3:
                if (i6 >= tlv_len) {
                    return false;
                }
                char_6bit = data[i6] >> SIXBIT_CASE3_SHIFT;
                i6++;
                break;
            default: return false;
        }

        char_6bit &= SIXBIT_ASCII_MASK;
        dst[i]     = char_6bit + SIXBIT_ASCII_SPACE_OFFSET;
    }

    return true;
}

}  // namespace verif

namespace {

// Fixed sizes: decode_6bit_ascii groups 4 chars per 3 input bytes;
// 4-char output + 1 spare input byte → triggers EOF boundary checks.
constexpr uint8_t kDstLen = 4;
constexpr uint8_t kSrcLen = 4;

// ----- Phase 1: totality -----

void f_checksum_total()
{
    uint8_t buf[8] = {nondet_u8(), nondet_u8(), nondet_u8(), nondet_u8(),
                      nondet_u8(), nondet_u8(), nondet_u8(), nondet_u8()};
    (void)verif::verify_checksum(buf, 8);
}

void f_decode_total()
{
    uint8_t src[kSrcLen] = {nondet_u8(), nondet_u8(), nondet_u8(), nondet_u8()};
    uint8_t dst[kDstLen]{};
    uint8_t tlv_len = nondet_u8();
    __ESBMC_assume(tlv_len <= kSrcLen);
    (void)verif::decode_6bit_ascii(src, tlv_len, dst, kDstLen);
}

// ----- Phase 2: functional contracts -----

void f_checksum_single_byte()
{
    uint8_t v      = nondet_u8();
    bool    result = verif::verify_checksum(&v, 1);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(result == (v == 0),
                   "verify_checksum({v}): true iff v == 0");
#endif
    (void)result;
}

void f_checksum_two_bytes()
{
    uint8_t a = nondet_u8();
    uint8_t b = nondet_u8();
    uint8_t buf[2] = {a, b};
    bool    result = verif::verify_checksum(buf, 2);

#ifdef ESBMC_FUNCTIONAL
    uint8_t expected_sum = static_cast<uint8_t>(a + b);
    __ESBMC_assert(result == (expected_sum == 0),
                   "verify_checksum({a,b}): true iff (a+b)&0xFF == 0");
#endif
    (void)result;
}

void f_decode_output_range()
{
    // When decode_6bit_ascii succeeds, every output byte is in [0x20, 0x5F]
    // (space..underscore — the printable 6-bit ASCII range).
    uint8_t src[kSrcLen] = {nondet_u8(), nondet_u8(), nondet_u8(), nondet_u8()};
    uint8_t dst[kDstLen]{};

    bool ok = verif::decode_6bit_ascii(src, kSrcLen, dst, kDstLen);

#ifdef ESBMC_FUNCTIONAL
    if (ok) {
        for (uint8_t i = 0; i < kDstLen; i++) {
            __ESBMC_assert(dst[i] >= verif::SIXBIT_ASCII_SPACE_OFFSET,
                           "decode_6bit_ascii: output byte >= 0x20");
            __ESBMC_assert(
                dst[i] <= verif::SIXBIT_ASCII_SPACE_OFFSET + verif::SIXBIT_ASCII_MASK,
                "decode_6bit_ascii: output byte <= 0x5F");
        }
    }
#endif
    (void)ok;
}

}  // namespace

int main()
{
    switch (nondet_uint() % 5) {
        case 0: f_checksum_total();        break;
        case 1: f_decode_total();          break;
        case 2: f_checksum_single_byte();  break;
        case 3: f_checksum_two_bytes();    break;
        case 4: f_decode_output_range();   break;
    }
    return 0;
}
