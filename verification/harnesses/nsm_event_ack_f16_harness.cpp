// F-16: is_event_ack_enable missing bounds check on event_id
//
// nsm.cpp:1089-1106 — is_event_ack_enable(NsmMsgType, uint8_t event_id) computes
//   ByteIndex = event_id / 8
//   return (type0/6_event_ack_bitmask.at(ByteIndex) & ...) != 0;
//
// type0_event_ack_bitmask and type6_event_ack_bitmask are both
// std::array<uint8_t, NvMctpEventSupportedNum=8>. For event_id ∈ [64, 255],
// ByteIndex ∈ [8, 31] — out of range for a size-8 array. No bounds guard is
// present; same asymmetric-guard pattern as F-5 (set_bit/unset_bit) and
// F-15 (is_event_source_enable).
//
// is_event_ack_enable is called from nsm_event.cpp:96 inside the event-sending
// path, making this potentially reachable from the NSM event dispatch loop.
//
// Expected: VERIFICATION FAILED ("Index out of bounds") on either msg_type path.

#include "nv/mctp/nsm_msg_bitmask.h"
#include <array>
#include <cstdint>

using namespace nv::mctp::nsm_msg;

extern "C" { uint8_t nondet_u8(); unsigned nondet_uint(); }

int main()
{
    // Model type0_event_ack_bitmask / type6_event_ack_bitmask
    // (both are std::array<uint8_t, NvMctpEventSupportedNum=8> members of Nsm)
    std::array<uint8_t, NvMctpEventSupportedNum> type0_ack_bitmask{};
    std::array<uint8_t, NvMctpEventSupportedNum> type6_ack_bitmask{};

    uint8_t event_id = nondet_u8();
    __ESBMC_assume(event_id >= 64);   // ByteIndex = event_id/8 >= 8 → OOB

    const size_t ByteIndex = event_id / 8;   // ∈ [8, 31] for event_id ∈ [64, 255]
    const size_t BitOffset = event_id % 8;

    // Cover both branches of is_event_ack_enable (nsm.cpp:1091 and 1097)
    if (nondet_uint() % 2 == 0)
        (void)(type0_ack_bitmask.at(ByteIndex) & (1U << BitOffset));  // DeviceCapabilityDiscovery path
    else
        (void)(type6_ack_bitmask.at(ByteIndex) & (1U << BitOffset));  // Firmware path

    return 0;
}
