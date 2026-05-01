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
// Status of the "full dispatch path" upgrade (post esbmc/esbmc#4215,#4233,#4235):
//   - platforms::Control ctrl{} constructs cleanly (esbmc#4214 fixed by #4215).
//   - ctrl.on_set_endpoint_id(rx, tx) via VerifControl (using Control::on_set_endpoint_id)
//     still crashes to_solver_smt_ast (smt_ast.h:111) even after #4235:
//       switch(static_cast<SetEndpoint>(crx.data[0])) — the switch discriminant
//       is a bit_cast member, and the SMT encoding produces a null AST pointer.
//       #4235 fixed fall-through label normalisation but not the discriminant
//       encoding crash.  This crash is currently untracked upstream.
//   - ctrl.process(rx, tx) is also blocked by dereference.cpp:1358 on the
//     variable-index _routing_map.at(entry_in_map) loop in
//     on_get_routing_table_entry (dead code on a SetEpId packet, but inlined).
//
// WORKAROUND: VerifControl::call_on_set_endpoint_id() inlines the semantics of
// on_set_endpoint_id() with if-else instead of switch, which avoids the SMT
// crash and produces correct VERIFICATION FAILED (224 VCCs,
// counterexample: iface_val=2, valid=true, cur_eid.at(2) OOB).
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
using pdk::mctp::app::PacketType;
using pdk::mctp::app::SetEndpoint;
using pdk::mctp::app::Validator;
using pdk::mctp::platforms::Control;
using pdk::mctp::platforms::Interface;
using pdk::mctp::platforms::get_packet_interface;
using pdk::mctp::platforms::set_cur_eid;
using pdk::mctp::platforms::set_packet_interface;

// Thin subclass to expose the protected on_set_endpoint_id() logic.
// WORKAROUND: production on_set_endpoint_id uses
// switch(static_cast<SetEndpoint>(crx.data[0])) as the discriminant; with
// crx obtained via std::bit_cast this crashes ESBMC's SMT encoding
// (to_solver_smt_ast, smt_ast.h:111) even after #4233 and #4235.
// This if-else is semantically equivalent and avoids the crash.
struct VerifControl : Control {
    void call_on_set_endpoint_id(const Packet& rx, Packet& tx)
    {
        auto& crx = pdk::mctp::app::Control::PktReq::from(rx);
        if (pdk::mctp::app::Control::get_packet_type(rx) == PacketType::Request) {
            auto sub = static_cast<SetEndpoint>(crx.data[0]);
            if (sub == SetEndpoint::SetEidNormal || sub == SetEndpoint::SetEidForced) {
                set_cur_eid(_router, get_packet_interface(rx), crx.data[1]);
            }
        }
        (void)tx;
    }
};

constexpr auto UsEnd = static_cast<uint8_t>(Interface::UsEnd);
constexpr auto IfEnd = static_cast<uint8_t>(Interface::End);

int main()
{
    // Gap interface: passes validate()'s ">= End" guard but OOBs cur_eid (size 2).
    uint8_t iface_val = nondet_u8();
    __ESBMC_assume(iface_val >= UsEnd && iface_val < IfEnd);

    VerifControl ctrl{};
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
    if (valid) {
        Packet tx{};
        ctrl.call_on_set_endpoint_id(rx, tx);
    }

    return 0;
}
