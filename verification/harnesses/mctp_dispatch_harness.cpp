// ESBMC harness proving F-1 end-to-end with the correct packet type.
//
// A Control SetEpId Request with interface in the gap [Interface::UsEnd=2,
// Interface::End=18) passes Validator::validate() — the validator guards
// against `>= End` (18) but cur_eid is only sized to UsEnd (2).
//
// This is the packet type that actually reaches set_cur_eid() in production:
//   Control::process() dispatches Cmd::SetEpId to on_set_endpoint_id()
//   (pdk-mctp-platforms-control.cpp:33,61), which calls
//   set_cur_eid(_router, get_packet_interface(rx), crx.data[1])
//   unconditionally for SetEidNormal/SetEidForced sub-commands.
//
// Status of the "full dispatch path" upgrade (post esbmc/esbmc#4215):
//   - platforms::Control ctrl{} now constructs without crashing (#4214 fixed).
//   - ctrl.process(rx, tx) crashes dereference.cpp:1358 in construct_from_
//     const_struct_offset, triggered by on_get_routing_table_entry's variable-
//     index loop (dead code on a SetEpId packet but still inlined by ESBMC).
//   - ctrl.on_set_endpoint_id(rx, tx) (via thin subclass) crashes mk_eq in
//     bitwuzla_conv.cpp:512 / z3_conv.cpp:756 with a bitvector-width mismatch:
//     switch-on-static_cast<enum>(packed_field) + second field read in case
//     body (filed as esbmc/esbmc#4216).
//
// Until #4216 is fixed this harness calls set_cur_eid() directly after
// validate(), relying on code inspection for step (3): on_set_endpoint_id()
// calls set_cur_eid() unconditionally without a bounds check on the interface.
// The harness formally proves (1) and (2).
//
// Expected: VERIFICATION FAILED — "std::array::at out of range"

#include <cstdint>

#include "app/pdk-mctp-app-enums.h"
#include "app/pdk-mctp-app-packet-plat.h"
#include "app/pdk-mctp-app-router-plat.h"
#include "app/pdk-mctp-app-validator.h"

extern "C" {
uint8_t nondet_u8();
}

using pdk::mctp::app::Cmd;
using pdk::mctp::app::MsgType;
using pdk::mctp::app::NULL_EID;
using pdk::mctp::app::Packet;
using pdk::mctp::app::SetEndpoint;
using pdk::mctp::app::Validator;
using pdk::mctp::platforms::Interface;
using pdk::mctp::platforms::RoutingTable;
using pdk::mctp::platforms::set_cur_eid;
using pdk::mctp::platforms::set_packet_interface;

constexpr auto UsEnd = static_cast<uint8_t>(Interface::UsEnd);
constexpr auto IfEnd = static_cast<uint8_t>(Interface::End);

int main()
{
    // Gap interface: passes validate()'s ">= End" guard but OOBs cur_eid (size 2).
    uint8_t iface_val = nondet_u8();
    __ESBMC_assume(iface_val >= UsEnd && iface_val < IfEnd);

    RoutingTable router{};
    Validator    v{router};

    // Control SetEpId Request — the packet type that on_set_endpoint_id()
    // handles.  Satisfies every guard in validate():
    //   hdr_ver=1, dst_eid=NULL_EID (eid_ok), som=1, eom=1,
    //   msg_type=Control, tag_owner=1+rq=1+d=0 → PacketType::Request,
    //   command_code=SetEpId, data[0]=SetEidNormal.
    Packet rx{};
    rx.hdr.hdr_ver   = 0x1;
    rx.hdr.dst_eid   = NULL_EID;
    rx.hdr.som       = 1;
    rx.hdr.eom       = 1;
    rx.hdr.tag_owner = 1;
    rx.msg[0]        = static_cast<uint8_t>(MsgType::Control);
    rx.msg[1]        = 0x80;  // rq=1 (bit 7), d=0 (bit 6)
    rx.msg[2]        = static_cast<uint8_t>(Cmd::SetEpId);
    rx.msg[3]        = static_cast<uint8_t>(SetEndpoint::SetEidNormal);
    rx.msg[4]        = nondet_u8();  // EID to assign
    set_packet_interface(rx, iface_val);

    // Step 1: validate() returns true — the validator gap.
    bool valid = v.validate(rx, static_cast<Interface>(iface_val));

    // Step 2: on_set_endpoint_id() calls set_cur_eid() with the raw interface
    // byte (pdk-mctp-platforms-control.cpp:61).  cur_eid.at(iface_val) OOBs.
    if (valid) {
        set_cur_eid(router, iface_val, rx.msg[4]);
    }

    return 0;
}
