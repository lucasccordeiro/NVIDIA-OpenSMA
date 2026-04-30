// Minimal reproducer for a residual mk_eq crash after esbmc/esbmc#4233.
//
// Pattern: switch on static_cast<enum>(member) where the member is accessed
// via std::bit_cast from a packed struct, AND the case body calls
// std::array::at() with a value from another packed struct member.
//
// The if-else equivalent of the switch works correctly (produces
// VERIFICATION FAILED — OOB at::at()); the switch form crashes:
//
//   Assertion failed: (a->sort->get_data_width() == b->sort->get_data_width()),
//   function mk_eq, file bitwuzla_conv.cpp, line 512
//
// This uses the actual production types from pdk-mctp-app-control.h and
// pdk-mctp-platforms-router.h.  A standalone struct reproduction with
// equivalent member layout does NOT crash; the issue appears to be in how
// ESBMC encodes the switch guard when the discriminant comes from a
// bit_cast-obtained packed struct with bitfield base members.
//
// Workaround: replace switch with if-else in verification harnesses.
// Fixed by: TODO (pending upstream fix)
//
// Expected: VERIFICATION FAILED ("std::array::at out of range").
// Actual:   mk_eq assertion crash at bitwuzla_conv.cpp:512.
//
// Reproduction (requires production MCTP headers on include path):
//   esbmc --std c++20 --overflow-check \
//     -I../stubs \
//     -I<repo>/corepdk/modules/mctp-cpp/src \
//     -I<repo>/corepdk/modules/mctp-cpp/src/platforms/x86 \
//     switch_bitcast_at_crash.cpp \
//     <repo>/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp

#include <cstdint>
#include "app/pdk-mctp-app-control.h"
#include "app/pdk-mctp-app-packet-plat.h"
#include "app/pdk-mctp-app-router-plat.h"
#include "pdk-mctp-platforms-control.h"

extern "C" { uint8_t nondet_u8(); }

int main()
{
    pdk::mctp::app::Packet rx{};
    rx.hdr.tag_owner = 1;
    rx.msg[3] = static_cast<uint8_t>(pdk::mctp::app::SetEndpoint::SetEidNormal);
    uint8_t iface = nondet_u8();
    pdk::mctp::platforms::set_packet_interface(rx, iface);

    auto& crx = pdk::mctp::app::Control::PktReq::from(rx);
    pdk::mctp::platforms::RoutingTable router{};

    switch (static_cast<pdk::mctp::app::SetEndpoint>(crx.data[0])) {
        case pdk::mctp::app::SetEndpoint::SetEidNormal:
        case pdk::mctp::app::SetEndpoint::SetEidForced:
            router.ec.cur_eid.at(pdk::mctp::platforms::get_packet_interface(rx)) = crx.data[1];
            break;
        default: break;
    }
    return 0;
}
