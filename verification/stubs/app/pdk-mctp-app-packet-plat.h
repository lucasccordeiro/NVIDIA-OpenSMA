// Verification-only overlay of corepdk/modules/mctp-cpp/src/app/pdk-mctp-app-packet-plat.h
//
// Required because the production plat.h contains a relative `#include
// "pdk-mctp-app-packet.h"` that resolves to the production packet.h regardless
// of `-I` order, which in turn re-introduces the symbols our overlay
// (workaround for esbmc/esbmc#4180) already defines, producing a redefinition
// error. By also overlaying plat.h, the relative include resolves to our
// overlay packet.h that lives next to it under verification/stubs/app/.
//
// The function declarations below are byte-identical to production.
//
// Remove once esbmc/esbmc#4180 is fixed.
#pragma once
#include "pdk-mctp-app-packet.h"

namespace pdk::mctp::platforms {

Packet::InterfaceType get_packet_interface(const app::Packet& pkt);
Packet::LengthType    get_packet_length(const app::Packet& pkt);
Packet::InterfaceType get_packet_interface(const PrivateHeader& priv);
Packet::LengthType    get_packet_length(const PrivateHeader& priv);

void set_packet_interface(app::Packet& pkt, Packet::InterfaceType interface);
void set_packet_length(app::Packet& pkt, Packet::LengthType len);
void set_packet_interface(PrivateHeader& priv, Packet::InterfaceType interface);
void set_packet_length(PrivateHeader& priv, Packet::LengthType len);

}  // namespace pdk::mctp::platforms
