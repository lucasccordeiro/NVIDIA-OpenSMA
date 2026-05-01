// Minimal reproducer: value-initialising a struct that inherits from a class
// via {} crashes ESBMC's SMT encoding.
//
// Trigger: `Derived d{}` where Derived is a struct and Base is a class.
// Workaround: `Derived d;` (default-init) avoids the crash.
//
// Crash: Assertion failed: (r), function to_solver_smt_ast, file smt_ast.h, line 111
//
// Expected: VERIFICATION FAILED ("OOB").
// Actual:   crash at to_solver_smt_ast, smt_ast.h:111.
//
// Reproduction:
//   esbmc --std c++20 struct_brace_init_crash.cpp

#include <cstdint>

extern "C" { uint8_t nondet_u8(); }

class Base {
    uint8_t _x{};
protected:
    void check(uint8_t i) { __ESBMC_assert(i < _x, "OOB"); }
public:
    Base() = default;
};

struct Derived : Base {
    using Base::check;
};

int main()
{
    Derived d{};   // {} triggers crash; `Derived d;` works correctly
    uint8_t i = nondet_u8();
    __ESBMC_assume(i >= 4);
    d.check(i);
    return 0;
}
