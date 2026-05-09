// ESBMC verification stub for nv/common/expected.h.
//
// Minimal Expected<T, E> sufficient for the call sites in nv/ipc/event.h
// that consumers of the verification harness exercise:
//   - implicit construction from T
//   - value() accessor (throwing-style; harness never calls on error)
//   - has_value() probe
//
// The production type uses NV_TRY/NV_FOE machinery; that's irrelevant for
// the harness because the stubbed Event::bits() always returns a value.
#pragma once
#include <cstdint>

namespace nv::common {

template<typename T, typename E>
class Expected
{
public:
    constexpr Expected(T value) : _value(value), _has(true) {}
    constexpr Expected(E /*err*/) : _value{}, _has(false) {}

    constexpr bool has_value() const { return _has; }
    constexpr T    value() const { return _value; }
    constexpr T    value_or(T fallback) const { return _has ? _value : fallback; }

    constexpr explicit operator bool() const { return _has; }
    constexpr T        operator*() const { return _value; }

private:
    T    _value;
    bool _has;
};

}  // namespace nv::common
