// ESBMC verification stub for nv/flash/flash.h
// Flash::get_data returns Error (no flash in verification); set_data no-ops.
#pragma once
#include "nv/flash/common.h"

namespace nv::flash {

class Flash {
public:
    // Address and Data are both uint32_t in the verification stub, so only one
    // overload of each is needed — duplicate signatures would be a redeclaration.
    static Status get_data(Key, Data&)  { return Status::Error; }
    static Status set_data(Key, Data)   { return Status::Error; }
};

}  // namespace nv::flash
