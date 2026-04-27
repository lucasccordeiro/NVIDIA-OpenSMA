// ESBMC harness for pdk::mctp::app::Validator::validate
// (corepdk/modules/mctp-cpp/src/app/pdk-mctp-app-validator.cpp).
//
// Phase 1 (default): language-level safety on attacker-controlled input.
//   The validator dispatches on packet header bit-fields. ESBMC must show
//   no OOB read on _router.ec.cur_eid (sized UsEnd == 2) when the harness
//   sweeps every uint8_t for `interface` (the Interface enum guard inside
//   validate() is what's being checked).
//
// Phase 2 (-DESBMC_FUNCTIONAL=1): functional contract.
//   - reset() zeroes the state machine.
//   - validate() returns false whenever interface >= Interface::End.
//   - validate() returns false whenever ctl.hdr_ver != 0x01.

#include <cstdint>

#include "app/pdk-mctp-app-validator.h"

extern "C" {
uint8_t  nondet_u8();
uint16_t nondet_u16();
unsigned nondet_uint();
}

using pdk::mctp::app::Packet;
using pdk::mctp::app::Validator;
using pdk::mctp::platforms::Interface;
using pdk::mctp::platforms::RoutingTable;

constexpr auto IfEnd = static_cast<uint16_t>(Interface::End);

namespace {

void check_validate_safe_for_any_interface()
{
    // Phase 1: validate() must never OOB-deref / overflow regardless of the
    // interface byte the caller passes. uint8_t covers 0..255, including
    // values >= IfEnd which the validator must reject internally.
    RoutingTable router{};
    Validator    v{router};

    Packet      pkt{};
    const auto  iface_byte = nondet_u8();
    const auto  iface      = static_cast<Interface>(iface_byte);

    bool ok = v.validate(pkt, iface);

#ifdef ESBMC_FUNCTIONAL
    // Phase 2: out-of-range interfaces must be rejected.
    if (iface_byte >= IfEnd) {
        __ESBMC_assert(!ok, "validate rejects iface >= End");
    }
#else
    (void)ok;
#endif
}

void check_validate_rejects_bad_hdr_ver()
{
    // The first thing validate() checks is hdr_ver == 0x01. Sweep hdr_ver
    // and confirm Phase 1 safety; with ESBMC_FUNCTIONAL the rejection is a
    // contract.
    RoutingTable router{};
    Validator    v{router};

    Packet pkt{};
    pkt.hdr.hdr_ver = nondet_u8() & 0xF;       // hdr_ver is a 4-bit field
    __ESBMC_assume(pkt.hdr.hdr_ver != 0x01);   // bad headers only

    bool ok = v.validate(pkt, Interface::UsI2c);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(!ok, "validate rejects hdr_ver != 0x01");
#endif
    (void)ok;
}

void check_reset_zeroes_state()
{
    // Phase 1 + 2: reset() should leave all internal counters at 0 / Control.
    RoutingTable router{};
    Validator    v{router};
    v.reset();

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(v._pkt_seq == 0, "reset clears pkt_seq");
    __ESBMC_assert(v._msg_tag == 0, "reset clears msg_tag");
    __ESBMC_assert(v._eid == 0, "reset clears eid");
#endif
}

}  // namespace

int main()
{
    switch (nondet_uint() % 3) {
        case 0: check_validate_safe_for_any_interface(); break;
        case 1: check_validate_rejects_bad_hdr_ver(); break;
        case 2: check_reset_zeroes_state(); break;
    }
    return 0;
}
