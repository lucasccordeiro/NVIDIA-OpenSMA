// ESBMC negative harness for F-12: decode_6bit_ascii output not bounded to printable ASCII.
//
// Finding: decode_6bit_ascii (fru.cpp line 169) produces output bytes via:
//   char_6bit = (data[i6] >> FRU_TLV_TYPE_SHIFT) | (data[i6+1] << SIXBIT_CASE1_SHIFT);
//   char_6bit &= SIXBIT_ASCII_MASK;   // mask to 6 bits: result in [0, 63]
//   dst[i]     = char_6bit + SIXBIT_ASCII_SPACE_OFFSET;  // [0x20, 0x5F]
//
// For case 1 (byte_pos == 1): the expression before the mask is:
//   (data[i6] >> 6) | (data[i6+1] << 2)
// Both operands are uint8_t, promoted to int. The shift left 2 on data[i6+1] can produce
// values > 0x3F before the mask clears the high bits. After masking, the value is in [0, 63].
// Adding 0x20 gives [0x20, 0x5F] — this should always be printable.
//
// However, the existing fru_utils_harness (with ESBMC_FUNCTIONAL) already checks this
// contract and verifies it. This negative harness asserts WITHOUT the mask to confirm
// that the mask IS necessary and that without it the output would escape [0x20, 0x5F].
//
// More precisely: the harness inlines decode_6bit_ascii verbatim from fru.cpp but
// asserts each output byte is in [0x20, 0x5F]. Since the implementation applies the mask
// correctly, this harness should confirm whether the property holds or fails.
//
// The real question: does the production decode_6bit_ascii ALWAYS produce output in
// [0x20, 0x5F]? The mask `char_6bit &= 0x3F` ensures char_6bit ∈ [0, 63], so
// dst[i] = char_6bit + 0x20 ∈ [0x20, 0x5F]. This appears correct.
//
// BUT: in case 1 and case 2, the intermediate computation before the mask assignment uses
// bitwise-OR of shifted values. If the mask `char_6bit &= SIXBIT_ASCII_MASK` is applied
// AFTER the assignment to char_6bit at line 98, the result is always ≤ 0x3F.
//
// This harness targets the actual claim: "nondet input can produce output outside [0x20, 0x5F]".
// Expected: VERIFICATION FAILED if the claim is true; VERIFICATION SUCCESSFUL if not.

#include <cstdint>

extern "C" {
uint8_t  nondet_u8();
unsigned nondet_uint();
}

// Constants verbatim from fru.cpp
constexpr uint8_t SIXBIT_ASCII_SPACE_OFFSET = 0x20;
constexpr uint8_t SIXBIT_ASCII_MASK         = 0x3F;
constexpr uint8_t SIXBIT_CHARS_PER_GROUP    = 4;
constexpr uint8_t SIXBIT_CASE1_SHIFT        = 2;
constexpr uint8_t SIXBIT_CASE2_SHIFT        = 4;
constexpr uint8_t SIXBIT_CASE3_SHIFT        = 2;
constexpr uint8_t FRU_TLV_TYPE_SHIFT        = 6;

// Verbatim from fru.cpp lines 169-222 (dst uses a fixed-size array instead of span)
static bool decode_6bit_ascii(const uint8_t* data, uint8_t tlv_len,
                               uint8_t* dst, uint16_t dst_len)
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

        char_6bit &= SIXBIT_ASCII_MASK;  // mask to 6 bits
        dst[i]     = char_6bit + SIXBIT_ASCII_SPACE_OFFSET;
    }

    return true;
}

// Smallest group that exercises all four byte_pos cases: 4 chars from 3 input bytes.
constexpr uint8_t kDstLen = 4;
constexpr uint8_t kSrcLen = 4;

int main()
{
    uint8_t src[kSrcLen] = {nondet_u8(), nondet_u8(), nondet_u8(), nondet_u8()};
    uint8_t dst[kDstLen] = {};

    const bool ok = decode_6bit_ascii(src, kSrcLen, dst, kDstLen);

    if (ok) {
        for (uint8_t i = 0; i < kDstLen; i++) {
            // Post-condition: every decoded character must be in printable ASCII [0x20, 0x5F]
            __ESBMC_assert(dst[i] >= SIXBIT_ASCII_SPACE_OFFSET,
                           "F-12: decoded char >= 0x20 (printable ASCII lower bound)");
            __ESBMC_assert(dst[i] <= SIXBIT_ASCII_SPACE_OFFSET + SIXBIT_ASCII_MASK,
                           "F-12: decoded char <= 0x5F (printable ASCII upper bound)");
        }
    }

    return 0;
}
