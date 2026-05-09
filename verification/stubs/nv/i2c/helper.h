// ESBMC verification stub for nv/i2c/helper.h.
//
// Production crc8 iterates byte-by-byte over std::span; for the ssif harness
// the actual CRC value is irrelevant — what matters is that PEC validity is
// modelled as a free choice so both `pec valid` and `pec invalid` branches
// in smbus_block_write are explored.
//
// Returning nondet_u8() achieves that without paying for the loop unwind
// (production's i2c_crc8 target verifies the real implementation separately).
#pragma once
#include <cstdint>
#include <span>

extern "C" uint8_t nondet_u8();

namespace nv::i2c {

// Production signatures take std::span<const uint8_t>. ESBMC's bundled
// <span> doesn't synthesize the span<T> → span<const T> conversion the
// production toolchain provides, so the stub overloads on both forms.
// All four return nondet_u8() so PEC validity is unconstrained.
inline uint8_t crc8(std::span<const uint8_t> /*data*/) { return nondet_u8(); }
inline uint8_t crc8(std::span<uint8_t>       /*data*/) { return nondet_u8(); }
inline uint8_t crc8(uint8_t /*crc*/, std::span<const uint8_t> /*data*/) { return nondet_u8(); }
inline uint8_t crc8(uint8_t /*crc*/, std::span<uint8_t>       /*data*/) { return nondet_u8(); }

}  // namespace nv::i2c
