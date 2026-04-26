// Verification-only no-op shim for pdk/cmn/log/log.h.
// The production header drags in <utility>, <limits>, source-location
// machinery, debug-level templates, persistent log plat::log_p, and a
// requires-clause concept — none of which we want to inflict on ESBMC for
// modules that only use logging incidentally.
//
// API surface preserved: pdk::cmn::log::Destination enum, here{}, hide{},
// and the templated fatal/error/warn/debug/info methods that take an
// optional Destination as a template parameter.
#pragma once

namespace pdk::cmn::log {

enum class Destination
{
    Console    = 1,
    Persistent = 2,
    Both       = 3,
};

namespace internal {
struct Base
{
    Base() = default;

    template <Destination D = Destination::Both, typename... Args>
    int fatal(const char*, Args&&...) { return 0; }
    template <Destination D = Destination::Both, typename... Args>
    int error(const char*, Args&&...) { return 0; }
    template <Destination D = Destination::Both, typename... Args>
    int warn(const char*, Args&&...)  { return 0; }
    template <Destination D = Destination::Both, typename... Args>
    int debug(const char*, Args&&...) { return 0; }
    template <Destination D = Destination::Both, typename... Args>
    int info(const char*, Args&&...)  { return 0; }
};
}  // namespace internal

struct here : internal::Base { here() = default; };
struct hide : internal::Base { hide() = default; };

}  // namespace pdk::cmn::log
