// ESBMC harness proving F-1 end-to-end through the real dispatch path.
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
// Status of the "full dispatch path" upgrade:
//   - platforms::Control ctrl{} constructs cleanly (esbmc#4214 fixed by #4215).
//   - ctrl.on_set_endpoint_id(rx, tx) via VerifControl thin subclass works
//     correctly after esbmc#4232 (fixed by #4233) and esbmc#4234 (fixed by #4235).
//   - ctrl.process(rx, tx) is still blocked: on_get_routing_table_entry's
//     variable-index _routing_map.at(entry_in_map) loop crashes
//     dereference.cpp:1358 (dead code on a SetEpId packet but inlined by ESBMC).
//
// VerifControl is a thin subclass whose only purpose is to promote the
// protected on_set_endpoint_id() to public so the harness can call it directly.
// The production source (pdk-mctp-platforms-control.cpp) is compiled as-is —
// no if-else workaround needed since #4235 fixed the switch-case normalisation.
// WORKAROUND esbmc#4237: `VerifControl ctrl;` (default-init) not `ctrl{}` —
// value-initialising a struct that inherits from a class crashes
// to_solver_smt_ast (smt_ast.h:111).
//
// Expected: VERIFICATION FAILED — "std::array::at out of range"

#include <cstdint>

#include "app/pdk-mctp-app-control.h"
#include "app/pdk-mctp-app-enums.h"
#include "app/pdk-mctp-app-packet-plat.h"
#include "app/pdk-mctp-app-router-plat.h"
#include "app/pdk-mctp-app-validator.h"
#include "pdk-mctp-platforms-control.h"

extern "C" {
uint8_t nondet_u8();
}

using pdk::mctp::app::Cmd;
using pdk::mctp::app::MsgType;
using pdk::mctp::app::NULL_EID;
using pdk::mctp::app::Packet;
using pdk::mctp::app::SetEndpoint;
using pdk::mctp::app::Validator;
using pdk::mctp::platforms::Control;
using pdk::mctp::platforms::Interface;
using pdk::mctp::platforms::set_packet_interface;

// Thin subclass to promote the protected on_set_endpoint_id() to public.
struct VerifControl : Control {
    using Control::on_set_endpoint_id;
};

constexpr auto UsEnd = static_cast<uint8_t>(Interface::UsEnd);
constexpr auto IfEnd = static_cast<uint8_t>(Interface::End);

int main()
{
    // Gap interface: passes validate()'s ">= End" guard but OOBs cur_eid (size 2).
    uint8_t iface_val = nondet_u8();
    __ESBMC_assume(iface_val >= UsEnd && iface_val < IfEnd);

    VerifControl ctrl;
    Validator    v{ctrl.router()};

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
    Packet tx{};
    if (valid) {
        ctrl.on_set_endpoint_id(rx, tx);
    }

    return 0;
}
