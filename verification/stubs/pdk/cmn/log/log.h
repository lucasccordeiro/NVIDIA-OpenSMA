// Verification-only no-op shim for pdk/cmn/log/log.h.
// The production header drags in <utility>, <limits>, source-location
// machinery, debug-level templates, persistent log plat::log_p, and a
// requires-clause concept — none of which we want to inflict on ESBMC for
// modules that only use logging incidentally.
//
// API surface preserved: `pdk::cmn::log::here().info(fmt, args...)` and
// the equivalent fatal/error/warn/debug/info methods on `here`/`hide`.
#pragma once

namespace pdk::cmn::log {

struct here
{
    here() = default;

    template <typename... Args> int fatal(const char*, Args&&...) { return 0; }
    template <typename... Args> int error(const char*, Args&&...) { return 0; }
    template <typename... Args> int warn (const char*, Args&&...) { return 0; }
    template <typename... Args> int debug(const char*, Args&&...) { return 0; }
    template <typename... Args> int info (const char*, Args&&...) { return 0; }
};

struct hide
{
    hide() = default;

    template <typename... Args> int fatal(const char*, Args&&...) { return 0; }
    template <typename... Args> int error(const char*, Args&&...) { return 0; }
    template <typename... Args> int warn (const char*, Args&&...) { return 0; }
    template <typename... Args> int debug(const char*, Args&&...) { return 0; }
    template <typename... Args> int info (const char*, Args&&...) { return 0; }
};

}  // namespace pdk::cmn::log
