// ESBMC harness for pdk::mctp::app::Packet.
//
// Phase 1 (default): language-level safety.
//   to_span() round-trip + get/set helpers should not deref nullptr,
//   overflow, or read OOB. ESBMC's default checks (pointer/bounds/div-by-zero)
//   plus the Makefile's overflow/memory-leak/NaN flags cover the suite asked.
//
// Phase 2 (-DESBMC_FUNCTIONAL=1): functional contract.
//   get(set(x)) == x for both {length, interface} accessor pairs;
//   round-trip via to_span/from preserves all fields.
//
// All deeply-qualified names are pulled into harness scope via using-decls
// (workaround for esbmc/esbmc#4180).

#include <cstdint>

#include "app/pdk-mctp-app-packet.h"
#include "app/pdk-mctp-app-packet-plat.h"

extern "C" {
uint8_t  nondet_u8();
uint16_t nondet_u16();
unsigned nondet_uint();
}

// WORKAROUND esbmc#4180: type-alias instead of using-declaration for class
// types — ESBMC's converter rejects `Using UsingType` sugar over RecordType.
using Packet = pdk::mctp::app::Packet;
using pdk::mctp::platforms::PrivHeaderSize;
using pdk::mctp::platforms::get_packet_interface;
using pdk::mctp::platforms::get_packet_length;
using pdk::mctp::platforms::set_packet_interface;
using pdk::mctp::platforms::set_packet_length;

namespace {

void check_to_span_self_consistency()
{
    Packet pkt{};
    auto   sp = pkt.to_span();
    __ESBMC_assert(sp.size() == sizeof(Packet), "to_span size");
    __ESBMC_assert(sp.data() != nullptr, "to_span data not null");
    volatile uint8_t sink = sp[sp.size() - 1];
    (void)sink;
}

void check_get_set_helpers()
{
    Packet     pkt{};
    const auto len_in = nondet_u16();
    const auto if_in  = nondet_u8();

    set_packet_length(pkt, len_in);
    set_packet_interface(pkt, if_in);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(get_packet_length(pkt) == len_in, "len round-trip");
    __ESBMC_assert(get_packet_interface(pkt) == if_in, "iface round-trip");
    __ESBMC_assert(get_packet_length(pkt.priv) == len_in, "len priv overload");
    __ESBMC_assert(get_packet_interface(pkt.priv) == if_in, "iface priv overload");
#endif
    (void)get_packet_length(pkt);
    (void)get_packet_interface(pkt);
}

void check_round_trip()
{
    Packet     pkt{};
    const auto len_in = nondet_u16();
    const auto if_in  = nondet_u8();
    set_packet_length(pkt, len_in);
    set_packet_interface(pkt, if_in);

    auto    sp     = pkt.to_span();
    Packet& mirror = Packet::from(sp);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(get_packet_length(mirror) == len_in, "rt len");
    __ESBMC_assert(get_packet_interface(mirror) == if_in, "rt iface");
    __ESBMC_assert(sp.size() == PrivHeaderSize + sizeof(pkt.hdr) + sizeof(pkt.msg),
                   "span layout matches priv+hdr+msg");
#endif
    (void)mirror;
}

}  // namespace

int main()
{
    switch (nondet_uint() % 3) {
        case 0: check_to_span_self_consistency(); break;
        case 1: check_get_set_helpers(); break;
        case 2: check_round_trip(); break;
    }
    return 0;
}
