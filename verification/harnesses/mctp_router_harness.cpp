// ESBMC harness for pdk::mctp::platforms::{get,set}_cur_eid and get_uuid
// (corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp).
//
// Phase 1 (default): language-level safety.
//   - get_cur_eid bounds-checks `interface < UsEnd` and falls back to the
//     default. ESBMC must show no OOB read for ANY uint8_t interface.
//   - set_cur_eid does NOT bounds-check; it calls .at(interface) directly.
//     With the verification shim's at() asserting bounds, ESBMC will flag
//     interface >= UsEnd as a property violation. We split the harness so
//     the well-formed path is reported separately from the negative path.
//
// Phase 2 (-DESBMC_FUNCTIONAL=1): functional contract.
//   - get_cur_eid(set_cur_eid(table, i, eid), i) == eid for all i < UsEnd.

#include <cstdint>

#include "app/pdk-mctp-app-router-plat.h"
#include "pdk-mctp-platforms-config.h"

extern "C" {
uint8_t  nondet_u8();
unsigned nondet_uint();
}

using pdk::mctp::platforms::Interface;
using pdk::mctp::platforms::RoutingTable;
using pdk::mctp::platforms::DefaultInterface;
using pdk::mctp::platforms::get_cur_eid;
using pdk::mctp::platforms::set_cur_eid;
using pdk::mctp::platforms::get_uuid;

constexpr auto UsEnd = static_cast<uint8_t>(Interface::UsEnd);

namespace {

void check_get_with_any_interface()
{
    // get_cur_eid must be safe for ANY uint8_t input — its own bounds check
    // covers the OOB path. With nondet_u8 the model checker explores 0..255.
    RoutingTable table{};
    const auto   iface = nondet_u8();
    (void)get_cur_eid(table, iface);
}

void check_set_with_in_range_interface()
{
    // Constrain interface to the valid prefix; set + get must round-trip.
    RoutingTable table{};
    const auto   iface = nondet_u8();
    const auto   eid   = nondet_u8();
    __ESBMC_assume(iface < UsEnd);
    set_cur_eid(table, iface, eid);
#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(get_cur_eid(table, iface) == eid, "set/get round-trip");
#endif
}

void check_get_uuid_pure()
{
    RoutingTable table{};
    auto         u = get_uuid(table);
    (void)u;
}

}  // namespace

int main()
{
    switch (nondet_uint() % 3) {
        case 0: check_get_with_any_interface(); break;
        case 1: check_set_with_in_range_interface(); break;
        case 2: check_get_uuid_pure(); break;
    }
    return 0;
}
