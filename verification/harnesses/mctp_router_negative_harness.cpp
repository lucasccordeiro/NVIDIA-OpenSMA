// Negative ESBMC harness for pdk::mctp::platforms::set_cur_eid.
//
// set_cur_eid() does NOT bounds-check its `interface` argument; it calls
// routing_table.ec.cur_eid.at(interface) directly. With our std::array shim
// asserting bounds via __ESBMC_assert, calling set_cur_eid with
// interface >= UsEnd must produce a counterexample.
//
// This is more than a model-checking artefact: in production at() throws
// std::out_of_range, and the throw is not caught anywhere on this call path
// (corepdk uses no exception-catching infra in this code). The end result
// would be std::terminate(). The negative test therefore documents a real
// callee-side robustness gap.

#include <cstdint>

#include "app/pdk-mctp-app-router-plat.h"

using pdk::mctp::platforms::RoutingTable;
using pdk::mctp::platforms::Interface;
using pdk::mctp::platforms::set_cur_eid;

constexpr auto UsEnd = static_cast<uint8_t>(Interface::UsEnd);

int main()
{
    RoutingTable table{};
    // Pin to UsEnd: caller passes an out-of-range interface index.
    set_cur_eid(table, UsEnd, /*eid=*/0xAB);  // <-- expected violation point
    return 0;
}
