// ESBMC harness for nv::mctp::nsm_msg bitmask operations
// (production: src/nv/mctp/nsm_msg_bitmask.h).
//
// nsm_msg_bitmask.h has no external dependencies (only <array> and <cstdint>)
// so we include it directly from the production tree.
//
// Phase 1: verify set_bit, unset_bit, get_bit, is_bit_set are safe for all
//          nondet uint8_t positions on the 32-element (NvMctpSupportedNum)
//          array. For pos ∈ [0,255], byte_index = pos/8 ∈ [0,31] which is
//          always within the 32-element array — VERIFICATION SUCCESSFUL.
//
// Phase 2: functional contracts — set then get returns non-zero; unset then
//          get returns zero; is_bit_set iff get_bit != 0.

#include "nv/mctp/nsm_msg_bitmask.h"
#include <cstdint>

extern "C" {
uint8_t  nondet_u8();
unsigned nondet_uint();
}

using namespace nv::mctp::nsm_msg;

namespace {

// ----- Phase 1: safety for the large (32-element) bitmask -----

void f_set_bit32_safe()
{
    std::array<uint8_t, NvMctpSupportedNum> bm{};
    uint8_t pos = nondet_u8();
    set_bit(bm, pos);
}

void f_unset_bit32_safe()
{
    std::array<uint8_t, NvMctpSupportedNum> bm{};
    uint8_t pos = nondet_u8();
    unset_bit(bm, pos);
}

void f_get_bit32_safe()
{
    std::array<uint8_t, NvMctpSupportedNum> bm{};
    uint8_t pos = nondet_u8();
    (void)get_bit(bm, pos);
}

void f_is_bit_set32_safe()
{
    std::array<uint8_t, NvMctpSupportedNum> bm{};
    uint8_t pos = nondet_u8();
    (void)is_bit_set(bm, pos);
}

// ----- Phase 1: safety for the small (8-element) bitmask, pos < 64 -----

void f_get_bit8_safe_all()
{
    // get_bit has a bounds guard — safe for any pos.
    std::array<uint8_t, NvMctpEventSupportedNum> bm{};
    uint8_t pos = nondet_u8();
    (void)get_bit(bm, pos);
}

void f_set_bit8_safe_inrange()
{
    // set_bit on 8-element array is safe iff pos < 64 (byte_index < 8).
    std::array<uint8_t, NvMctpEventSupportedNum> bm{};
    uint8_t pos = nondet_u8();
    __ESBMC_assume(pos < 64);
    set_bit(bm, pos);
}

void f_unset_bit8_safe_inrange()
{
    // unset_bit on 8-element array has the same structure as set_bit — no
    // bounds guard — so the safe region is also pos < 64.
    std::array<uint8_t, NvMctpEventSupportedNum> bm{};
    uint8_t pos = nondet_u8();
    __ESBMC_assume(pos < 64);
    unset_bit(bm, pos);
}

// ----- Phase 2: functional contracts -----

void f_contract_set_then_get()
{
    // After set_bit(bm, pos), get_bit(bm, pos) must be non-zero.
    std::array<uint8_t, NvMctpSupportedNum> bm{};
    uint8_t pos = nondet_u8();
    set_bit(bm, pos);
#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(get_bit(bm, pos) != 0, "set_bit then get_bit: bit is set");
    __ESBMC_assert(is_bit_set(bm, pos),   "set_bit then is_bit_set: bit is set");
#endif
}

void f_contract_unset_then_get()
{
    // After set_bit then unset_bit, get_bit must return zero.
    std::array<uint8_t, NvMctpSupportedNum> bm{};
    uint8_t pos = nondet_u8();
    set_bit(bm, pos);
    unset_bit(bm, pos);
#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(get_bit(bm, pos) == 0, "unset_bit then get_bit: bit is clear");
    __ESBMC_assert(!is_bit_set(bm, pos),  "unset_bit then is_bit_set: bit is clear");
#endif
}

void f_contract_is_bit_set_iff_get()
{
    // is_bit_set(bm, pos) iff get_bit(bm, pos) != 0.
    // Only the target byte needs to be nondet — both functions read at most
    // one byte, so initialising only bm.at(pos/8) covers all reachable states.
    std::array<uint8_t, NvMctpSupportedNum> bm{};
    uint8_t pos = nondet_u8();
    bm.at(pos / 8) = nondet_u8();  // nondet target byte; no loop needed
#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(is_bit_set(bm, pos) == (get_bit(bm, pos) != 0),
                   "is_bit_set agrees with get_bit");
#else
    (void)is_bit_set(bm, pos);
    (void)get_bit(bm, pos);
#endif
}

}  // namespace

int main()
{
    switch (nondet_uint() % 10) {
        case 0: f_set_bit32_safe();              break;
        case 1: f_unset_bit32_safe();            break;
        case 2: f_get_bit32_safe();              break;
        case 3: f_is_bit_set32_safe();           break;
        case 4: f_get_bit8_safe_all();           break;
        case 5: f_set_bit8_safe_inrange();       break;
        case 6: f_unset_bit8_safe_inrange();     break;
        case 7: f_contract_set_then_get();       break;
        case 8: f_contract_unset_then_get();     break;
        case 9: f_contract_is_bit_set_iff_get(); break;
    }
    return 0;
}
