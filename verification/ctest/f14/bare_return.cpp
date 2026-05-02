// F-14 failure-mode demonstrator: Pca9555::i2c_write bare return on Input command.
//
// Production: Pca9555::i2c_write (pca9555.cpp lines 142-175):
//   for (uint8_t i = start_index; i < data_length; i++) {
//       switch (CommandRegister) {
//           case Input: {
//               return;    // <-- bare return exits the entire function, not just switch
//           }
//           case Output: { ... break; }
//           ...
//       }
//   }
// For a 3-byte write (data_length = 3, start_index = 1), two iterations are
// expected (i = 1 and i = 2). When CommandRegister == Input, the bare return
// fires at i = 1 (zero iterations complete) and all remaining bytes are silently
// dropped.
//
// Expected runtime behaviour: assert fires because completed (0) != expected (2).

#include <cassert>
#include <cstdint>

extern "C" {
unsigned char __VERIFIER_nondet_uchar(void);
void          __VERIFIER_assume(int cond);
}

enum Pca9555Register : uint8_t { Input = 0, Output = 1, Invert = 2, Direction = 3 };

// Verbatim loop logic from pca9555.cpp lines 118-175 (propagate_* calls stripped
// since the bare return fires before any I2C write would occur).
static unsigned simulate_i2c_write_iterations(uint8_t cmd_byte, uint8_t data_length,
                                               const uint8_t* data_bytes)
{
    const auto CommandRegister = static_cast<Pca9555Register>(cmd_byte / 2);
    if (data_length == 1) return 0;
    uint8_t start_index = 1;
    if (data_length > 3) start_index = static_cast<uint8_t>(data_length - 2);

    unsigned iterations = 0;
    for (uint8_t i = start_index; i < data_length; i++) {
        (void)data_bytes[i];
        switch (CommandRegister) {
            case Input:
                return iterations;  // bare return — models production
            case Output: case Invert: case Direction: default:
                iterations++;
                break;
        }
    }
    return iterations;
}

int main()
{
    // cmd_byte = 0x00 → cmd_byte / 2 = 0 → CommandRegister = Input
    uint8_t buf[3] = { 0x00, __VERIFIER_nondet_uchar(), __VERIFIER_nondet_uchar() };

    const unsigned completed = simulate_i2c_write_iterations(buf[0], 3, buf);
    const unsigned expected  = 2;  // start_index=1, data_length=3 → i=1 and i=2

    assert(completed == expected &&
           "F-14: bare return on Input command drops remaining data bytes");
    return 0;
}
