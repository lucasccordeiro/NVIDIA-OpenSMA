// ESBMC 8.2.0 — false-positive bounds/alignment check on [[gnu::packed]] bitfield constructor
//
// A [[gnu::packed]] struct whose constructor initialises a bitfield member via
// a member-initialiser-list triggers a spurious VCC:
//   dereference failure: Access to object out of bounds
// The struct is valid C++; the false positive is produced by ESBMC's bounds model
// treating the bitfield initialiser as an out-of-bounds pointer write.
//
// Reproduce:
//   esbmc --std c++20 --overflow-check packed_bitfield_ctor_bounds_fp.cpp
// Expected: VERIFICATION SUCCESSFUL
// Actual:   VERIFICATION FAILED (dereference failure: Access to object out of bounds)
//
// Workaround: --no-bounds-check (or --no-align-check on some variants).
// Filed as esbmc#<TBD>.

#include <cstdint>

struct [[gnu::packed]] S {
    uint32_t flag : 1;
    uint32_t rest : 31;
    S() : flag{0}, rest{0} {}
};

int main()
{
    S s;
    (void)s.flag;
    return 0;
}
