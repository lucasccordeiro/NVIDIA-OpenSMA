// Minimal reproducer for esbmc/esbmc#4216.
//
// ESBMC crashes with:
//   Assertion failed: (a->sort->get_data_width() == b->sort->get_data_width()),
//   function mk_eq, file bitwuzla_conv.cpp:512  (also z3_conv.cpp:756)
//
// Trigger: struct derived from a bitfield base + array member.  Using
//   static_cast<enum>(struct.data[0]) as a switch discriminant AND reading
//   a second field of the same struct inside the matching case alongside a VCC
//   produces bitvector expressions of mismatched widths in ESBMC's SMT encoding.
//
// Removing `(void)req.data[1]` from the case body → VERIFICATION FAILED correctly.
// Using `if (req.data[0] == 0)` instead of switch → VERIFICATION FAILED correctly.
//
// Expected: VERIFICATION FAILED ("OOB").
// Actual:   assertion in mk_eq (bitwuzla/z3_conv.cpp).
//
// Reproduction:
//   esbmc --std c++20 mkeq_width_mismatch.cpp

#include <cstdint>

enum class E : uint8_t { A = 0 };

struct Base { uint8_t hi : 4; uint8_t lo : 4; };
struct Req : Base { uint8_t data[2]; };

extern "C" uint8_t nondet_u8();

int main()
{
    uint8_t idx = nondet_u8();
    __ESBMC_assume(idx >= 1);

    Req req{};
    req.data[1] = nondet_u8();

    switch (static_cast<E>(req.data[0])) {
        case E::A:
            __ESBMC_assert(idx < 1, "OOB");
            (void)req.data[1];  // without this line, no crash
            break;
        default: break;
    }
    return 0;
}
