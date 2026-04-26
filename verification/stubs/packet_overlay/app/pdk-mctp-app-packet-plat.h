// Verification-only overlay of pdk-mctp-app-packet-plat.h.
//
// Required because the production plat.h does a relative
// `#include "pdk-mctp-app-packet.h"` that resolves to the production
// packet.h regardless of `-I` order; without this overlay, both the
// production and our overlay packet.h get pulled into the same TU, causing
// redefinition errors. Function signatures below are byte-identical to
// production. Remove together with packet.h overlay once esbmc#4180 part 1
// is fixed.
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
