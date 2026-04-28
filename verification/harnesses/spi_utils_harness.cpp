// ESBMC harness for nv::spi byte-buffer (de)serialisation.
// Production: src/nv/spi/utils.{h,cpp}.
//
// utils.h pulls nv/ctimer/ctimer.h (hardware timer machinery) for
// transitive reasons; the four functions verified here use only <span>
// + the production ByteShift{1,2,3} constants. Inlined verbatim from
// production for the same reason as nsm_type_2 / utils harnesses.
//
// Phase 1: confirm OOB-safety on any (buf_size, start_idx) pair. The
//   functions guard against `start_idx + sizeof > buf.size()`.
// Phase 2: round-trip via u*_to_buf -> buf_to_u* recovers the value;
//   byte-order is big-endian (high byte at lowest index).

#include <cstdint>
#include <span>

extern "C" {
uint8_t  nondet_u8();
uint16_t nondet_u16();
uint32_t nondet_u32();
unsigned nondet_uint();
}

namespace verif {

constexpr uint8_t ByteShift1 = 8;
constexpr uint8_t ByteShift2 = 16;
constexpr uint8_t ByteShift3 = 24;

// Verbatim from src/nv/spi/utils.h. The C++20 [expr.shift]/2 modular
// rule for non-negative E1 makes `(uint32_t)(buf[i] << ByteShift3)`
// well-defined; with the type-driven non-negativity refinement of
// the --overflow-check skip (esbmc#4201 fix), the production form
// verifies without parenthesisation gymnastics.
inline uint16_t buf_to_u16(std::span<uint8_t> buf, uint8_t start_idx)
{
    if (start_idx + sizeof(uint16_t) > buf.size()) {
        return 0;
    }
    return (static_cast<uint16_t>(buf[start_idx] << ByteShift1))
         | (static_cast<uint16_t>(buf[start_idx + 1]));
}

inline uint32_t buf_to_u32(std::span<uint8_t> buf, uint8_t start_idx)
{
    if (start_idx + sizeof(uint32_t) > buf.size()) {
        return 0;
    }
    return (static_cast<uint32_t>(buf[start_idx] << ByteShift3))
         | (static_cast<uint32_t>(buf[start_idx + 1] << ByteShift2))
         | (static_cast<uint32_t>(buf[start_idx + 2] << ByteShift1))
         | (static_cast<uint32_t>(buf[start_idx + 3]));
}

// Verbatim from src/nv/spi/utils.cpp.
inline void u16_to_buf(std::span<uint8_t> buf, uint16_t value, uint8_t start_idx)
{
    if (start_idx + sizeof(uint16_t) > buf.size()) {
        return;
    }
    buf[start_idx]     = (uint8_t)((value >> ByteShift1) & UINT8_MAX);
    buf[start_idx + 1] = (uint8_t)((value) & UINT8_MAX);
}

inline void u32_to_buf(std::span<uint8_t> buf, uint32_t value, uint8_t start_idx)
{
    if (start_idx + sizeof(uint32_t) > buf.size()) {
        return;
    }
    buf[start_idx]     = (uint8_t)((value >> ByteShift3) & UINT8_MAX);
    buf[start_idx + 1] = (uint8_t)((value >> ByteShift2) & UINT8_MAX);
    buf[start_idx + 2] = (uint8_t)((value >> ByteShift1) & UINT8_MAX);
    buf[start_idx + 3] = (uint8_t)((value) & UINT8_MAX);
}

}  // namespace verif

namespace {

// Bound the buffer size to keep ESBMC's symex tractable; 8 bytes is
// strictly larger than max(sizeof(u16), sizeof(u32)) and exercises the
// boundary for both directions.
constexpr unsigned long BufN = 8;

void f_buf_to_u16_total()
{
    uint8_t storage[BufN] = {};
    for (unsigned i = 0; i < BufN; ++i) storage[i] = nondet_u8();
    std::span<uint8_t> buf{storage, BufN};
    uint8_t            idx = nondet_u8();
    (void)verif::buf_to_u16(buf, idx);
}

void f_buf_to_u32_total()
{
    uint8_t storage[BufN] = {};
    for (unsigned i = 0; i < BufN; ++i) storage[i] = nondet_u8();
    std::span<uint8_t> buf{storage, BufN};
    uint8_t            idx = nondet_u8();
    (void)verif::buf_to_u32(buf, idx);
}

void f_u16_to_buf_total()
{
    uint8_t            storage[BufN] = {};
    std::span<uint8_t> buf{storage, BufN};
    uint16_t           v   = nondet_u16();
    uint8_t            idx = nondet_u8();
    verif::u16_to_buf(buf, v, idx);
}

void f_u32_to_buf_total()
{
    uint8_t            storage[BufN] = {};
    std::span<uint8_t> buf{storage, BufN};
    uint32_t           v   = nondet_u32();
    uint8_t            idx = nondet_u8();
    verif::u32_to_buf(buf, v, idx);
}

void f_u16_round_trip()
{
    uint8_t            storage[BufN] = {};
    std::span<uint8_t> buf{storage, BufN};
    uint16_t           v   = nondet_u16();
    uint8_t            idx = nondet_u8();
    __ESBMC_assume(idx + sizeof(uint16_t) <= BufN);  // in-bounds path

    verif::u16_to_buf(buf, v, idx);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(verif::buf_to_u16(buf, idx) == v, "u16 round-trip");
    // Big-endian: high byte at lowest index.
    __ESBMC_assert(buf[idx]     == (uint8_t)(v >> 8), "u16 BE high byte");
    __ESBMC_assert(buf[idx + 1] == (uint8_t)(v & 0xFF), "u16 BE low byte");
#endif
}

void f_u32_round_trip()
{
    uint8_t            storage[BufN] = {};
    std::span<uint8_t> buf{storage, BufN};
    uint32_t           v   = nondet_u32();
    uint8_t            idx = nondet_u8();
    __ESBMC_assume(idx + sizeof(uint32_t) <= BufN);

    verif::u32_to_buf(buf, v, idx);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(verif::buf_to_u32(buf, idx) == v, "u32 round-trip");
    __ESBMC_assert(buf[idx]     == (uint8_t)(v >> 24), "u32 BE byte 0");
    __ESBMC_assert(buf[idx + 1] == (uint8_t)((v >> 16) & 0xFF), "u32 BE byte 1");
    __ESBMC_assert(buf[idx + 2] == (uint8_t)((v >>  8) & 0xFF), "u32 BE byte 2");
    __ESBMC_assert(buf[idx + 3] == (uint8_t)(v & 0xFF), "u32 BE byte 3");
#endif
}

void f_oob_idx_no_write()
{
    // If start_idx is out of range, the writer must not modify the buffer.
    uint8_t storage[BufN]{};
    for (unsigned i = 0; i < BufN; ++i) storage[i] = 0xCD;  // sentinel
    std::span<uint8_t> buf{storage, BufN};
    uint8_t            idx = nondet_u8();
    __ESBMC_assume(idx + sizeof(uint32_t) > BufN);  // OOB

    verif::u32_to_buf(buf, nondet_u32(), idx);

#ifdef ESBMC_FUNCTIONAL
    for (unsigned i = 0; i < BufN; ++i) {
        __ESBMC_assert(buf[i] == 0xCD, "OOB idx leaves buffer untouched");
    }
#endif
}

}  // namespace

int main()
{
    switch (nondet_uint() % 7) {
        case 0: f_buf_to_u16_total(); break;
        case 1: f_buf_to_u32_total(); break;
        case 2: f_u16_to_buf_total(); break;
        case 3: f_u32_to_buf_total(); break;
        case 4: f_u16_round_trip(); break;
        case 5: f_u32_round_trip(); break;
        case 6: f_oob_idx_no_write(); break;
    }
    return 0;
}
