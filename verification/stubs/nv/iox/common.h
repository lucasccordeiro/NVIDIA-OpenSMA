// ESBMC verification stub for nv/iox/common.h
// Provides nv::iox::vrPort as an alias for InvalidGpioPort, matching the
// production definition used in nsm_type_5.cpp and nsm_event.h.
#pragma once
#include "nv/gpio/common.h"

namespace nv::iox {
constexpr nv::gpio::GpioPort vrPort = nv::gpio::InvalidGpioPort;
}  // namespace nv::iox
