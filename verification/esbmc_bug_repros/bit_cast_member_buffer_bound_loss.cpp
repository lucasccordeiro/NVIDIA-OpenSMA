// ESBMC 8.2.0 — spurious OOB on `std::copy` with a SYMBOLIC count, when
// the destination is reached through `*std::bit_cast<View*>(member.data())`
// inside a method that also reads through a parallel bit_cast'd source.
//
// Filed upstream: https://github.com/esbmc/esbmc/issues/4448
//
// This file is a placeholder: the bug only manifests in the full
// ssif.cpp context (smbus_block_write — switch on rx.cmd selecting
// among Write{Single,MultiStart,MultiMiddle,MultiEnd} branches, each
// calling std::copy_n with rx.size symbolic and the dest reached via
// `Packet::from(this->_buffer)`). Reduced reproducers attempted —
// constant-count copy, single-bit_cast direction, no-switch, no
// user-provided ctor — all VERIFY SUCCESSFUL, so the trigger involves
// some combination of:
//   - symbolic count satisfying a multi-step path condition
//   - both source and destination through bit_cast
//   - dest reached via a member-array of a class-with-user-provided-ctor
//   - intervening switch statement gating on a bit_cast-derived field
//
// To reproduce in this tree, temporarily re-enable the i2c_callback
// path in verification/harnesses/ssif_harness.cpp:
//
//   --- ssif_harness.cpp ---
//   // Drop in just before handle_tx():
//   {
//       sys::i2c::I2cSlaveBuffer buf{};
//       for (size_t i = 0; i < buf.size(); ++i) buf[i] = nondet_u8();
//       uint8_t address  = nondet_u8();
//       bool    is_read  = nondet_bool();
//       size_t  i2c_size; __ESBMC_assume(i2c_size <= buf.size());
//       Ssif::i2c_callback(address, is_read, buf, i2c_size, &ssif,
//                          nondet_bool());
//   }
//
// Then:
//   cd verification && ESBMC=/path/to/esbmc make ssif_safety
//
// Observed (ESBMC 8.2.0, Bitwuzla 0.9.0):
//   dereference failure: Access to object out of bounds
//   State 57: dest::2 = &ssif + 1     (inside bundled <algorithm> copy)
//   State trace shows i2c_size = 34 as the witness — a value that
//   passes the `i2c_size != rx.size + 2` reject path with rx.size = 32
//   then reaches WriteMulti{Middle,End} → std::copy_n.
//
// Expected: VERIFICATION SUCCESSFUL — every reachable write into
// `pkt.ipmi_data.begin() + offset` with offset+rx.size ≤ MaxPayloadSize
// (= 254) is in-bounds of pkt.ipmi_data (254 bytes), which is in-bounds
// of `_buffer` (UsbLstpMsgSize=512 bytes), which is a member of the
// statically-allocated `Ssif` instance.
//
// Bundled <bit> ships a pointer-to-pointer specialisation of bit_cast
// that uses reinterpret_cast (esbmc/src/cpp/library/bit:25-31), so the
// cast itself preserves the bit pattern — but ESBMC's pointer-
// provenance tracker drops the parent-buffer bounds across the cast
// AND across the subsequent member-array `.data()` projection, then
// bounds the result against a 1-byte view at `&ssif`.
//
// Same family as the workaround the c2c_mailbox commit references for
// pdk-mctp-app-packet.h (esbmc#4180 part 1 lineage).

#include <cstdint>

int main()
{
    // Standalone repro could not be reduced below the full
    // ssif_safety target. See block comment above for in-tree
    // reproduction steps; tracked upstream as esbmc#4448.
    return 0;
}
