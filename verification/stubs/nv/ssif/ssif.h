// ESBMC verification overlay for nv/ssif/ssif.h.
//
// Production ssif.h declares packed-struct factory methods (`from()`)
// whose inline bodies use `std::bit_cast`, `std::min`, and `std::span`,
// but the header itself does not pull in <bit>, <algorithm>, or <span>.
// Production gets away with this because ssif.cpp's preamble pulls them
// in before ssif.h is parsed in the only TU that consumes those bodies.
// ESBMC's frontend resolves inline bodies at the class-completion point,
// so it needs those headers visible upfront — including when the harness
// or the production .cpp itself is the TU.
//
// This overlay is found via -I$(STUBS) -I$(REPO)/src; it pre-includes the
// missing transitive headers then chains to the real production header
// via #include_next.
#pragma once

#include <algorithm>
#include <bit>
#include <span>
#include "nv/gpio/common.h"

#include_next "nv/ssif/ssif.h"
