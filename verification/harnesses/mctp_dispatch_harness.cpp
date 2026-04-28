// ESBMC harness proving F-1 reachability: Validator::validate() accepts a
// well-formed PLDM start-of-message whose interface index falls in the gap
// [Interface::UsEnd, Interface::End).  The validator's interface guard only
// rejects values >= Interface::End (18), but RoutingTable::ec.cur_eid has
// size Interface::UsEnd (2), so any gap interface [2, 17] that passes
// validation will OOB in the subsequent set_cur_eid() call.
//
// Expected: VERIFICATION FAILED — "std::array::at out of range" in
// cur_eid.at(iface_val) with iface_val in [2, 17].

#include <cstdint>

#include "app/pdk-mctp-app-enums.h"
#include "app/pdk-mctp-app-packet-plat.h"
#include "app/pdk-mctp-app-router-plat.h"
#include "app/pdk-mctp-app-validator.h"

extern "C" {
uint8_t  nondet_u8();
}

using pdk::mctp::app::MsgType;
using pdk::mctp::app::NULL_EID;
using pdk::mctp::app::Packet;
using pdk::mctp::app::Validator;
using pdk::mctp::platforms::Interface;
using pdk::mctp::platforms::RoutingTable;
using pdk::mctp::platforms::set_cur_eid;
using pdk::mctp::platforms::set_packet_interface;

// Interface::UsEnd == 2 (upstream-only map size)
// Interface::End   == 18 (full interface count)
constexpr auto UsEnd = static_cast<uint8_t>(Interface::UsEnd);
constexpr auto IfEnd = static_cast<uint8_t>(Interface::End);

int main()
{
    // Constrain the interface to the gap [UsEnd=2, End=18).  These values
    // pass the validator's `>= End` guard but index past the 2-entry cur_eid.
    uint8_t iface_val = nondet_u8();
    __ESBMC_assume(iface_val >= UsEnd && iface_val < IfEnd);

    RoutingTable router{};
    Validator    v{router};

    // Minimal PLDM start-of-message that satisfies every guard inside
    // validate(): hdr_ver==0x01, dst_eid==NULL_EID (eid_ok=true),
    // msg_type==Pldm, som==1.  This path returns true without calling
    // get_packet_type(), keeping the harness self-contained.
    Packet pkt{};
    pkt.hdr.hdr_ver = 0x1;
    pkt.hdr.dst_eid = NULL_EID;
    pkt.hdr.som     = 1;
    pkt.msg[0]      = static_cast<uint8_t>(MsgType::Pldm);
    set_packet_interface(pkt, iface_val);

    bool valid = v.validate(pkt, static_cast<Interface>(iface_val));

    // F-1: the validator approves the packet (valid == true for gap iface).
    // The production caller (on_set_endpoint_id) then calls set_cur_eid()
    // with the raw interface byte — cur_eid.at(iface_val) OOBs here.
    if (valid) {
        uint8_t eid = nondet_u8();
        set_cur_eid(router, iface_val, eid);
    }

    return 0;
}
