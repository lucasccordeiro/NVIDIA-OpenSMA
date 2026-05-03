// ESBMC verification stub for nv/debugtoken/debugtoken.h
#pragma once
#include <cstdint>

namespace nv {
namespace debugtoken {

enum class TokenErrorCode : uint16_t { Ok = 0, Error };

inline bool is_dbg_token_tlv_in_flash() { return false; }

}  // namespace debugtoken
}  // namespace nv
