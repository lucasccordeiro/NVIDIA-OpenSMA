// Structural harness for the DCD event-bitmask write handlers — sibling
// safety check to F-15/F-16 (read-side OOBs in is_event_source_enable /
// is_event_ack_enable on event_id ≥ 64).
//
// Source under test (nsm.cpp:983-1023 and 1096-1136). Both functions have
// the same shape after the length check + memcpy of the 9-byte
// NvMsgTypeWithEventBitmask body:
//
//   if (nv_msg_type == DeviceCapabilityDiscovery) {
//       type0_event_*_bitmask = bitmask;
//       for (size_t i = 0; i < NvMctpEventSupportedNum; ++i)
//           type0_event_*_bitmask.at(i) &= SupType0Event.at(i);
//       log_nvmsg_event_bitmask.at(static_cast<uint8_t>(nv_msg_type)) = false;  // set only
//   }
//   else if (nv_msg_type == Firmware) {
//       type6_event_*_bitmask = bitmask;
//       for (size_t i = 0; i < NvMctpEventSupportedNum; ++i)
//           type6_event_*_bitmask.at(i) &= SupType6Event.at(i);
//       log_nvmsg_event_bitmask.at(static_cast<uint8_t>(nv_msg_type)) = false;  // set only
//   }
//   else fill_error_packet(...);
//
// F-15/F-16 found OOBs because the read side did `bitmask.at(event_id / 8)`
// with event_id taken from the packet and bitmask.size() == 8 — index 8..31
// was reachable. The write side here uses
// `log_nvmsg_event_bitmask.at(uint8_t(nv_msg_type))` on a size-32 array,
// gated on nv_msg_type ∈ {DeviceCapabilityDiscovery=0, Firmware=6}, both
// well within bounds. The inner loops run i ∈ [0, 8) over size-8 arrays.
//
// Expected: VERIFICATION SUCCESSFUL — confirms the asymmetric-guard pattern
// that bit F-15/F-16 does NOT bite the write side.

#include "nv/mctp/enums.h"
#include "nv/mctp/nsm_msg_bitmask.h"
#include <array>
#include <cstdint>

using namespace nv::mctp;
using namespace nv::mctp::nsm_msg;

extern "C" uint8_t  nondet_u8();
extern "C" unsigned nondet_uint();

int main()
{
    // Production member sizes: nsm.h:1661-1666.
    std::array<uint8_t, NvMctpEventSupportedNum> type0_event_enable_bitmask{};
    std::array<uint8_t, NvMctpEventSupportedNum> type6_event_enable_bitmask{};
    std::array<uint8_t, NvMctpEventSupportedNum> type0_event_ack_bitmask{};
    std::array<uint8_t, NvMctpEventSupportedNum> type6_event_ack_bitmask{};
    std::array<uint8_t, NvMctpSupportedNum>      log_nvmsg_event_bitmask{};

    // Per-type supported-event masks — platform-defined byte arrays. Their
    // values do not affect bounds safety (the AND result is uint8_t either
    // way), so default-initialised to zero suffices.
    std::array<uint8_t, NvMctpEventSupportedNum> SupType0Event{};
    std::array<uint8_t, NvMctpEventSupportedNum> SupType6Event{};

    // Fully nondet packet body.
    auto nv_msg_type = static_cast<NsmMsgType>(nondet_u8());
    std::array<uint8_t, NvMctpEventSupportedNum> bitmask{};
    for (size_t i = 0; i < NvMctpEventSupportedNum; ++i)
        bitmask.at(i) = nondet_u8();

    // Drive both handler bodies via a nondet branch — single proof covers both.
    if (nondet_uint() % 2 == 0) {
        // on_dcd_set_current_event_srcs body (nsm.cpp:1002-1022, verbatim).
        if (nv_msg_type == NsmMsgType::DeviceCapabilityDiscovery) {
            type0_event_enable_bitmask = bitmask;
            for (size_t i = 0; i < NvMctpEventSupportedNum; ++i)
                type0_event_enable_bitmask.at(i) &= SupType0Event.at(i);
            log_nvmsg_event_bitmask.at(static_cast<uint8_t>(nv_msg_type)) = false;
        }
        else if (nv_msg_type == NsmMsgType::Firmware) {
            type6_event_enable_bitmask = bitmask;
            for (size_t i = 0; i < NvMctpEventSupportedNum; ++i)
                type6_event_enable_bitmask.at(i) &= SupType6Event.at(i);
            log_nvmsg_event_bitmask.at(static_cast<uint8_t>(nv_msg_type)) = false;
        }
    }
    else {
        // on_dcd_configure_event_ack body (nsm.cpp:1119-1135, verbatim).
        if (nv_msg_type == NsmMsgType::Firmware) {
            type6_event_ack_bitmask = bitmask;
            for (size_t i = 0; i < NvMctpEventSupportedNum; ++i)
                type6_event_ack_bitmask.at(i) &= SupType6Event.at(i);
        }
        else if (nv_msg_type == NsmMsgType::DeviceCapabilityDiscovery) {
            type0_event_ack_bitmask = bitmask;
            for (size_t i = 0; i < NvMctpEventSupportedNum; ++i)
                type0_event_ack_bitmask.at(i) &= SupType0Event.at(i);
        }
    }

    return 0;
}
