<!--
ESBMC issue draft — paste into https://github.com/esbmc/esbmc/issues/new

Title:
  Spurious OOB in std::copy when destination is `member_array.data()` reached
  via `std::bit_cast<View*>` and count is symbolic

Labels: bug, frontend, pointer-tracking
-->

## Summary

In ESBMC 8.2.0, `std::copy` (bundled `<algorithm>`, used internally by `std::copy_n`) reports a spurious **dereference failure: Access to object out of bounds** when its destination iterator is obtained as `member_array.data()` of an object reached through `*std::bit_cast<View*>(byte_buffer.data())`, and the copy length is symbolic. The bound check fires against a 1-byte view at the outer object's address (`&outer + 1`) even though the destination range is well within the underlying member buffer.

The bundled `<bit>` already specialises pointer-to-pointer `bit_cast` to use `reinterpret_cast` (`esbmc/src/cpp/library/bit:25-31`), so the bit pattern of the pointer itself survives the cast — but ESBMC's pointer-provenance tracker still drops the parent-buffer bounds across the cast and the subsequent member-array `.data()` projection.

## Why it matters

This is the universal firmware idiom for SMBus / MCTP / IPMI / PLDM parsers: reinterpret a fixed-size byte buffer as a packed protocol view, then `std::copy_n(...wire_payload..., count_from_wire, view.payload.begin())`. The same root cause is documented in the c2c_mailbox commit message of the OpenSMA tree, which references an existing in-tree workaround for `pdk-mctp-app-packet.h` (a verification-only overlay that replaces `std::bit_cast<T*>` with a C-cast at struct-decl scope).

## Environment

- ESBMC version: `ESBMC version 8.2.0 64-bit x86_64 linux`
- Solver: Bitwuzla 0.9.0 (default)
- Frontend: Clang 21.x (bundled with ESBMC release)
- C++ standard: `--std c++20`
- OS: Linux 6.8.0-106-generic
- Repro flags: `--overflow-check --unwind 36 --no-align-check`

## Reproducer

The bug only manifests in the full SSIF harness context. Reduced reproducers (constant count, single-direction `bit_cast`, no surrounding `switch`, no user-provided constructor) all `VERIFY SUCCESSFUL`. The combination of triggers appears to be:

1. Both **source** and **destination** of `std::copy` reached through `bit_cast<T*>`.
2. **Symbolic** copy length satisfying a non-trivial path condition (an inequality that admits multiple values).
3. Destination via `class-with-user-provided-ctor → member std::array<uint8_t, N>` (the source array can be a free static — the asymmetry matters).
4. An intervening `switch` on a field reached through the source `bit_cast`.

### In-tree reproduction (NVIDIA OpenSMA)

```bash
git clone https://github.com/openbmc/NVIDIA-OpenSMA   # or local checkout
cd NVIDIA-OpenSMA/verification

# Temporarily reactivate the data-callback path in ssif_harness.cpp
# (just before the handle_tx() call):

cat <<'PATCH' >> /tmp/ssif_repro.patch
*** verification/harnesses/ssif_harness.cpp
+   // Repro for esbmc#XXXX — re-enable the data callback.
+   {
+       sys::i2c::I2cSlaveBuffer buf{};
+       for (size_t i = 0; i < buf.size(); ++i) buf[i] = nondet_u8();
+       uint8_t address  = nondet_u8();
+       bool    is_read  = nondet_bool();
+       size_t  i2c_size; __ESBMC_assume(i2c_size <= buf.size());
+       Ssif::i2c_callback(address, is_read, buf, i2c_size, &ssif,
+                          nondet_bool());
+   }
PATCH

ESBMC=/path/to/esbmc make ssif_safety
```

### Observed output

```
State 57 file /tmp/esbmc-cpp-headers-XXXX/algorithm line 549 column 6
function copy thread 0
  dest::2 = &ssif + 1

State 58 file /tmp/esbmc-cpp-headers-XXXX/algorithm line 549 column 5
function copy thread 0
Violated property:
  file /tmp/esbmc-cpp-headers-XXXX/algorithm line 549 column 5 function copy
  dereference failure: Access to object out of bounds

VERIFICATION FAILED
```

The `dest` value `&ssif + 1` is wrong: the actual destination is

```
ssif._buffer.data() + offsetof(Packet, ipmi_data) + offset
= &ssif + offsetof(Ssif, _buffer) + 4 + offset
```

which lies in `ssif._buffer` (`UsbLstpMsgSize = 512` bytes) and therefore in `ssif` (≈ 600 bytes total).

### Production code being verified

The relevant fragment from `src/nv/ssif/ssif.cpp` (`smbus_block_write`):

```cpp
auto& rx  = SmbusWriteBlock::from(i2c_buffer.data());  // bit_cast<SmbusWriteBlock*>
// ... (size guards on rx.size, PEC check) ...
auto& pkt = Packet::from(_buffer);                     // bit_cast<Packet*>
switch (rx.cmd) {
    case WriteSingle: {
        std::copy_n(rx.data.begin(), rx.size, pkt.ipmi_data.begin());
        // ...
    }
    case WriteMultiStart: { ... std::copy_n(...); }
    case WriteMultiMiddle: { ... std::copy_n(..., pkt.ipmi_data.begin() + _rx_offset); }
    case WriteMultiEnd: { ... std::copy_n(..., pkt.ipmi_data.begin() + _rx_offset); }
}
```

The factory pattern:

```cpp
struct [[gnu::packed]] SmbusWriteBlock {
    uint8_t        cmd;
    uint8_t        size;
    PartDataBuffer data;            // std::array<uint8_t, 33>

    static SmbusWriteBlock& from(uint8_t* buffer) {
        return *std::bit_cast<SmbusWriteBlock*>(buffer);
    }
};

class Ssif {
    // ...
    using Buffer = std::array<uint8_t, nv::ipc::UsbLstpMsgSize>;  // 512 bytes
    Buffer _buffer;
    // ...
};
```

### Reduction attempts (all VERIFY SUCCESSFUL)

- Constant `count` argument to `std::copy` — sliced.
- `std::copy_n` written manually as `for (i=0; i<n; ++i) *dst++ = *src++;` — sliced.
- Single bit_cast direction (only dest, only source) — passes.
- No surrounding `switch` — passes.
- No user-provided ctor on the outer class — passes.
- Combinations of two of the above — pass.

The full ssif harness (with all four conditions present) reliably reproduces the OOB.

## Workaround in the affected tree

The OpenSMA tree had previously addressed the same family of issue for `pdk-mctp-app-packet.h` via a verification-only overlay header that pre-includes the production header but textually replaces `std::bit_cast<T*>` with a C-cast at struct-decl scope. See the `c2c_mailbox` commit message:

> the verification-only overlay for pdk-mctp-app-packet.h replaces
> std::bit_cast<T*> with a C-cast (tagged WORKAROUND esbmc#4180 part 1)
> — that delta is what keeps the memcpy out of the inductive proof obligation.

The same approach should unblock the SSIF data-path target.

## Suggested fix

When `bit_cast<To*>(from*)` is internally lowered to `reinterpret_cast`, ESBMC's pointer-tracker should retain (a) the original allocation as the parent and (b) the original size as the bound. Currently it appears to attach the bound `sizeof(To)` (or 1 byte in the observed CEX) at the cast site, then propagate that smaller bound through any member-array `.data()` projection that follows.

A subset of options:

1. In the bit_cast lowering, emit a `__ESBMC_assume(@object_size(result) >= @object_size(from))` so the parent allocation's size is preserved.
2. When the source pointer's allocation size is statically known to be larger than `sizeof(To)`, retain the source allocation as the result's bound.
3. Treat `member_array.data()` projection as inheriting the enclosing object's bound rather than the array's bound when the projection follows a `reinterpret_cast` that crossed type lines.

I'm happy to test patches against the in-tree reproducer.

## Related issues

- esbmc#4180 (part 1) — earlier fix for `bit_cast<T*>` interaction with memcpy in pdk-mctp.
- The c2c_mailbox commit (5c240dc-equivalent on the local tree) references the same family.
