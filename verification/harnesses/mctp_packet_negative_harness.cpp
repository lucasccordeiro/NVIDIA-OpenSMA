// Negative ESBMC harness for pdk::mctp::app::Packet::from().
//
// Expectation: ESBMC must report a counterexample on the corepdk_assert at
// the entry of from() when the input span is shorter than sizeof(Packet).
// A "VERIFICATION SUCCESSFUL" outcome here would mean the assert is dead or
// being optimised away — that itself is a bug we want to catch.
//
// We pin the input size to a value strictly less than sizeof(Packet) so the
// counterexample has a deterministic trigger; ESBMC still has to discover
// that the assert can fire. We do NOT __ESBMC_assume(size < sizeof(Packet))
// because the input buffer must be small enough that no reads past it occur
// — but the assertion fires before any read happens.

#include <cstdint>

#include "app/pdk-mctp-app-packet.h"

extern "C" {
unsigned nondet_uint();
}

using Packet = pdk::mctp::app::Packet;

int main()
{
    // Buffer too small for a Packet. The corepdk_assert in Packet::from()
    // must fire — ESBMC should report this as a property violation.
    constexpr unsigned long undersized = 4;
    static_assert(undersized < sizeof(Packet), "harness requires undersized");

    uint8_t            storage[undersized] = {};
    std::span<uint8_t> view{storage, undersized};

    Packet& p = Packet::from(view);  // <-- expected violation point
    (void)p;
    return 0;
}
