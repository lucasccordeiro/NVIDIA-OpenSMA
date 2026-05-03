// Verification stub for nv/nv.h
//
// The production header pulls in nv/common/debug.h and friends which
// transitively include platform-specific headers.  For verification we only
// need the nv::info / nv::warn / nv::error logging helpers used in the
// modules under test — all of which are no-ops here.
#pragma once

#include <cstdarg>

namespace nv {

// Variadic no-op stubs — the format string is a compile-time literal in every
// call site, so swallowing the arguments is safe and correct.
template <typename... Args>
inline void info([[maybe_unused]] const char* fmt, [[maybe_unused]] Args&&... args) {}

template <typename... Args>
inline void warn([[maybe_unused]] const char* fmt, [[maybe_unused]] Args&&... args) {}

template <typename... Args>
inline void error([[maybe_unused]] const char* fmt, [[maybe_unused]] Args&&... args) {}

// always_assert: no-op in verification (assertions are modelled separately).
inline void always_assert([[maybe_unused]] bool cond) {}

}  // namespace nv
