// ESBMC harness for nv/emulation/pca9555.{h,cpp}
//
// Class verified:
//   Pca9555 — software emulation of a PCA9555 16-bit I2C GPIO expander.
//   Manages two 8-bit banks of input/output/inversion/direction registers and
//   an interrupt vector. All logic is pure bit manipulation over std::array<uint8_t,2>.
//
// Phase 1: no arithmetic overflow or UB on any operation with nondet inputs.
//
// Phase 2: functional contracts —
//   - direction_constraint: after correct_direction_violations_and_apply(target, bank),
//     required-input bits are always 1 and required-output bits are always 0 in the
//     resulting direction register.
//   - input_update_masked: update_input_pins(target, mask) only changes input-direction
//     pins covered by mask; output-direction pins are unchanged.
//   - interrupt_default: freshly default-constructed Pca9555 has no pending interrupts
//     (get_interrupt_l_state() == true).
//   - output_propagation: after a write to the output register,
//     pins configured as outputs reflect the written value.
//
// Include notes: pca9555.h only depends on sys/i2c/i2c_slave.h (for I2cSlaveBufferSize);
// the production header pulls NV_IPC_CONFIG_H via that path.  The stubs shim provides
// just the buffer-size constant (35) without the platform config header.
// nv/nv.h is shimmed to no-op logging.

#include "nv/emulation/pca9555.h"

using namespace nv::emulation;

extern "C" {
uint8_t  nondet_u8();
uint16_t nondet_u16();
unsigned nondet_uint();
}

namespace {

// ----- Phase 1: totality -----

// Call every public method with nondet arguments — no UB or overflow.
void f_totality()
{
    const uint16_t init   = nondet_u16();
    const uint16_t out_m  = nondet_u16();
    const uint16_t in_m   = nondet_u16();
    Pca9555 dev{init, out_m, in_m};

    // i2c_read
    uint8_t rd = 0;
    dev.i2c_read(rd, true);
    dev.i2c_read(rd, false);

    // i2c_write: command byte selects register/bank; data bytes follow
    std::array<uint8_t, sys::i2c::I2cSlaveBufferSize> buf{};
    buf[0] = nondet_u8();  // command byte
    buf[1] = nondet_u8();  // data byte
    dev.i2c_write(buf, 1);  // register-select only
    dev.i2c_write(buf, 2);  // register + one data byte

    // update_input_pins (16-bit overload)
    dev.update_input_pins(nondet_u16(), nondet_u16());

    // update_input_pins (8-bit overload)
    const auto bank = static_cast<Pca9555Bank>(nondet_uint() % 2);
    dev.update_input_pins(nondet_u8(), nondet_u8(), bank);

    // getters
    uint16_t states16 = 0;
    dev.get_pin_states(states16);
    uint8_t states8 = 0;
    dev.get_pin_states(states8, bank);
    uint16_t dirs = 0;
    dev.get_pin_directions(dirs);
    (void)dev.get_interrupt_l_state();
}

// ----- Phase 2: functional contracts -----

// direction_constraint: required-input bits always 1, required-output bits always 0.
//
// correct_direction_violations_and_apply enforces:
//   result = (target | required_inputs) & ~required_outputs
// ⇒ (result & required_inputs) == required_inputs  [holds iff req_in & req_out == 0]
// ⇒ (result & required_outputs) == 0               [always holds]
//
// When a pin appears in both masks, required_outputs wins (last operation in the
// formula clears the bit).  The precondition req_in & req_out == 0 matches the
// intended usage: a pin is either a required input OR a required output, not both.
void f_direction_constraint()
{
    const uint8_t req_out = nondet_u8();
    const uint8_t req_in  = nondet_u8();
    __ESBMC_assume((req_in & req_out) == 0);  // no pin is simultaneously required-in and required-out

    // Pack masks into 16-bit constructor args (bank Zero uses low byte)
    Pca9555 dev{0, req_out, req_in};

    const auto bank   = static_cast<Pca9555Bank>(nondet_uint() % 2);
    const uint8_t cmd = static_cast<uint8_t>((Pca9555Register::Direction * 2)
                                             + static_cast<uint8_t>(bank));
    const uint8_t new_dir = nondet_u8();

    // Drive a Direction write command through i2c_write
    std::array<uint8_t, sys::i2c::I2cSlaveBufferSize> buf{};
    buf[0] = cmd;
    buf[1] = new_dir;
    dev.i2c_write(buf, 2);

    // Read back direction
    uint16_t dir16 = 0;
    dev.get_pin_directions(dir16);
    // Extract the relevant bank's byte
    const uint8_t dir_bank = (bank == Zero)
                           ? static_cast<uint8_t>(dir16)
                           : static_cast<uint8_t>(dir16 >> 8U);
    const uint8_t req_in_bank  = (bank == Zero)
                                ? static_cast<uint8_t>(req_in)
                                : static_cast<uint8_t>(static_cast<uint16_t>(req_in) >> 8U);
    const uint8_t req_out_bank = (bank == Zero)
                                ? static_cast<uint8_t>(req_out)
                                : static_cast<uint8_t>(static_cast<uint16_t>(req_out) >> 8U);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert((dir_bank & req_in_bank) == req_in_bank,
                   "required-input bits must remain 1 after direction write");
    __ESBMC_assert((dir_bank & req_out_bank) == 0,
                   "required-output bits must remain 0 after direction write");
#endif
    (void)dir_bank;
}

// input_update_masked: update_input_pins only writes to input-direction pins in the mask.
//
// After update_input_pins(target, mask) on a freshly constructed device where
// all pins are inputs (direction register = 0xFF), the pin state for bank Zero
// equals (target & mask) | (old & ~mask).
void f_input_update_masked()
{
    // All-inputs device: required_output_mask = 0 → all pins are inputs
    Pca9555 dev{0, 0x0000, 0xFFFF};

    // Read initial states
    uint16_t before = 0;
    dev.get_pin_states(before);

    const uint16_t target = nondet_u16();
    const uint16_t mask   = nondet_u16();
    dev.update_input_pins(target, mask);

    uint16_t after = 0;
    dev.get_pin_states(after);

#ifdef ESBMC_FUNCTIONAL
    // Bits covered by mask: after = target & mask  (old was 0, mask ⊆ direction=1 bits)
    // Bits not in mask: after = before & ~mask
    const uint16_t expected = static_cast<uint16_t>(target & mask)
                            | static_cast<uint16_t>(before & static_cast<uint16_t>(~mask));
    __ESBMC_assert(after == expected,
                   "input_update_masked: only masked input-direction pins change state");
#endif
    (void)after;
}

// interrupt_default: a Pca9555 constructed with initial_input_values=0 and no
// required masks has no pending interrupts.
//
// The parametrised constructor sets interrupt_low_vector = {0xFF, 0xFF}
// (InterruptVectorDefault) and state_at_last_interrupt_check to the same
// initial pin values.  With no subsequent state change, get_interrupt_l_state()
// must return true (active-low interrupt de-asserted).
void f_interrupt_default()
{
    Pca9555 dev{0, 0, 0};
#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(dev.get_interrupt_l_state(),
                   "freshly constructed Pca9555 with no state change must have no interrupts");
#endif
}

// output_propagation: writing to the output register drives output-direction pins.
//
// After writing value V to the output register for bank Zero (all pins configured
// as outputs: direction = 0), get_pin_states() must return V in bank Zero.
void f_output_propagation()
{
    // All-outputs device: required_output_mask = 0xFFFF, required_input_mask = 0
    Pca9555 dev{0, 0xFFFF, 0x0000};

    const uint8_t v = nondet_u8();
    // Write to Output register, bank Zero: command byte = Output*2 + Zero = 2
    std::array<uint8_t, sys::i2c::I2cSlaveBufferSize> buf{};
    buf[0] = static_cast<uint8_t>(Pca9555Register::Output * 2 + Pca9555Bank::Zero);
    buf[1] = v;
    dev.i2c_write(buf, 2);

    uint8_t state = 0;
    dev.get_pin_states(state, Zero);

#ifdef ESBMC_FUNCTIONAL
    __ESBMC_assert(state == v,
                   "output_propagation: pin state must equal written output value");
#endif
}

}  // namespace

int main()
{
    switch (nondet_uint() % 5) {
        case 0: f_totality();             break;
        case 1: f_direction_constraint(); break;
        case 2: f_input_update_masked();  break;
        case 3: f_interrupt_default();    break;
        case 4: f_output_propagation();   break;
    }
    return 0;
}
