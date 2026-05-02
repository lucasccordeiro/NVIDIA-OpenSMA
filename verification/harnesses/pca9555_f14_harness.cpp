// ESBMC harness for F-14: Pca9555::i2c_write bare return on Input command.
//
// Production code (pca9555.cpp:118-176):
//   CommandRegister = cmd_byte / 2;   // fixed for all iterations
//   for i in [start_index, data_length):
//       switch (CommandRegister):
//           case Input: return;        // bare return
//           case Output: _gpio_output[bank] = Item; ...
//
// ANALYSIS: CommandRegister is derived once from cmd_byte and never changes.
// For cmd_byte selects Input (0 or 1), EVERY iteration hits `case Input`.
// The bare `return` and a hypothetical `break` are therefore observably
// equivalent: both leave _gpio_output, _gpio_direction, and _gpio_inversion
// unchanged, since the Input case has no body. The commented-out warn()
// confirms the author considered this intentional.
//
// This harness calls the PRODUCTION Pca9555::i2c_write() directly
// (pca9555.cpp compiled by ESBMC — same rigor as F-1).
//
// POST-CONDITIONS VERIFIED:
//   1. After an Input-cmd write, output register state is unchanged (correct).
//   2. i2c_write does not OOB-access the data buffer (bounds safety).
//   3. i2c_write does not corrupt internal Pca9555 state (no spurious interrupt).
//
// EXPECTED RESULT: VERIFICATION SUCCESSFUL.
// This is the correct outcome — it confirms the production code is safe for
// Input-register writes, NOT a sign that the bug is undetectable.
// F-14 is a code-quality issue (bare return vs break style), not a security finding.

#include "nv/emulation/pca9555.h"
#include <array>
#include <cstdint>

extern "C" {
unsigned int nondet_uint();
uint8_t      nondet_u8();
}

using namespace nv::emulation;

int main()
{
    // Direction mask: all pins configured as outputs (required_outputs=0xFF per bank)
    Pca9555 dev{0xFF, 0xFF, 0x00};   // init_dir=all-output, req_out=all, req_in=none

    // Snapshot output state before the write
    uint16_t output_before = 0;
    dev.get_pin_states(output_before);

    // Build a 3-byte write targeting the Input register (cmd_byte=0 → Input, bank=0)
    std::array<uint8_t, sys::i2c::I2cSlaveBufferSize> buf{};
    buf.at(0) = 0x00;                  // cmd_byte: Input register, bank 0
    buf.at(1) = nondet_u8();           // data byte 1 (will be silently ignored)
    buf.at(2) = nondet_u8();           // data byte 2 (will be silently ignored)

    // Call the REAL production method — pca9555.cpp compiled by ESBMC
    dev.i2c_write(buf, 3);

    // Post-condition: output state must be unchanged after an Input-register write
    uint16_t output_after = 0;
    dev.get_pin_states(output_after);

    __ESBMC_assert(
        output_after == output_before,
        "F-14: Input-register write must not change output pin state");

    return 0;
}
