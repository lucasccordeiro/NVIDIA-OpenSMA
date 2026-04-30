// Minimal reproducer for esbmc/esbmc#4232 — Crash B.
//
// The function-call variant of the switch+bitfield-base crash still hits the
// original mk_eq assertion even after #4217:
//   Assertion failed: (a->sort->get_data_width() == b->sort->get_data_width()),
//   function mk_eq, file bitwuzla_conv.cpp:512  (also z3_conv.cpp:756)
//
// The only difference from mkeq_width_mismatch.cpp (Crash A) is that
// `req.data[1]` is consumed by a function call rather than `(void)`.  Crash A
// regressed to to_solver_smt_ast:111 after #4217; Crash B still hits mk_eq.
//
// Expected: VERIFICATION FAILED ("OOB").
// Actual:   assertion in mk_eq, bitwuzla_conv.cpp:512.
//
// Reproduction:
//   esbmc --std c++20 mkeq_width_mismatch_b.cpp

#include <cstdint>

enum class E : uint8_t { A = 0 };

struct Base { uint8_t hi : 4; uint8_t lo : 4; };
struct Req : Base { uint8_t data[2]; };

extern "C" uint8_t nondet_u8();
static void sink(uint8_t) {}

int main()
{
    uint8_t idx = nondet_u8();
    __ESBMC_assume(idx >= 1);

    Req req{};
    req.data[1] = nondet_u8();

    switch (static_cast<E>(req.data[0])) {
        case E::A:
            __ESBMC_assert(idx < 1, "OOB");
            sink(req.data[1]);  // function-call form — still mk_eq crash after #4217
            break;
        default: break;
    }
    return 0;
}
