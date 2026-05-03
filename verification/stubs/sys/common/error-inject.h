// ESBMC verification stub for sys/common/error-inject.h
#pragma once
#include <cstdlib>

namespace sys::common {

class ErrorInject
{
public:
    [[noreturn]] static void trigger_HardFault()     { abort(); }
    [[noreturn]] static void trigger_BusFault()      { abort(); }
    [[noreturn]] static void trigger_UsageFault()    { abort(); }
    [[noreturn]] static void trigger_SecureFault()   { abort(); }
    [[noreturn]] static void trigger_WatchdogReset() { abort(); }
    [[noreturn]] static void trigger_MemManageFault(){ abort(); }
};

}  // namespace sys::common
