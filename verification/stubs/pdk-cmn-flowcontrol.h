// Verification-only stub for pdk-cmn-flowcontrol.h.
// Replaces the real implementation that pulls in pdk/cmn/log/log.h and an
// Ada-exported corepdk_exit_program. Routes the assertion to ESBMC so failures
// surface as proper property violations instead of being silently logged.
#pragma once

#ifdef __ESBMC
#define HARNESS_ASSERT(c, m) __ESBMC_assert((c), (m))
#else
#include <cassert>
#define HARNESS_ASSERT(c, m) assert((c) && (m))
#endif

namespace pdk {
namespace cmn {
namespace flowcontrol {

inline void corepdk_assert(bool condition, const char* msg)
{
    HARNESS_ASSERT(condition, msg);
}

}  // namespace flowcontrol
}  // namespace cmn
}  // namespace pdk
