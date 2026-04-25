// F-1 failure-mode demonstrator (host build, mirrors production flags).
//
// Goal: observe what std::array<T, N>::at(i) does at runtime when i >= N
// under the production compile flags (-fno-exceptions -fno-rtti).
//
// The failure mode is what callers like Control::on_set_endpoint_id would
// see if the dispatch path delivers an interface index >= UsEnd to
// pdk::mctp::platforms::set_cur_eid. Reachability of that path is NOT
// proved by this test.

#include <array>
#include <cstdint>
#include <stdio.h>

extern "C" {
unsigned int __VERIFIER_nondet_uint(void);
void         __VERIFIER_assume(int cond);
}

// Mirror the production layout: cur_eid sized to UsEnd == 2 in the
// upstream config (UsI2c = 0, UsUsb = 1, UsEnd = 2).
static constexpr unsigned UsEnd = 2;

int main()
{
    std::array<uint8_t, UsEnd> cur_eid{};

    // ESBMC's ctest generator will pick concrete values; we constrain the
    // index to the "interesting" range [UsEnd, UsEnd + 8) so each test
    // case is a deliberate OOB write attempt.
    unsigned i = __VERIFIER_nondet_uint();
    __VERIFIER_assume(i >= UsEnd && i < UsEnd + 8);

    // The production line: routing_table.ec.cur_eid.at(interface) = eid;
    cur_eid.at(i) = 0xAB;

    // If we get here, at() did not abort. Print what happened.
    printf("survived: cur_eid[0]=%u cur_eid[1]=%u (i was %u)\n",
           cur_eid[0], cur_eid[1], i);
    return 0;
}
