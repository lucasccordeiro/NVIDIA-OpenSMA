// ESBMC negative harness for nv::mctp::nsm_msg::set_bit and unset_bit on the
// small (NvMctpEventSupportedNum = 8) bitmask array.
//
// set_bit / unset_bit on std::array<uint8_t, 8> call bitmask.at(pos/8)
// without a bounds guard. get_bit carries `if (byte_index < bitmask.size())`
// but set_bit and unset_bit do not. For pos ∈ [64, 255], byte_index = pos/8
// ∈ [8, 31] which is out-of-range for a size-8 array.
//
// Expected: VERIFICATION FAILED ("Index out of bounds") on either branch.
// Filed as F-5.

#include "nv/mctp/nsm_msg_bitmask.h"
#include <cstdint>

extern "C" { uint8_t nondet_u8(); unsigned nondet_uint(); }

using namespace nv::mctp::nsm_msg;

int main()
{
    std::array<uint8_t, NvMctpEventSupportedNum> bm{};
    uint8_t pos = nondet_u8();
    __ESBMC_assume(pos >= 64);     // byte_index = pos/8 >= 8 — out of range

    if (nondet_uint() % 2 == 0)
        set_bit(bm, pos);          // at(byte_index) OOB: F-5 set_bit path
    else
        unset_bit(bm, pos);        // at(byte_index) OOB: F-5 unset_bit path
    return 0;
}
