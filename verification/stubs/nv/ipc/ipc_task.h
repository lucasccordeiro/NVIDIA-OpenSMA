// ESBMC verification stub for nv/ipc/ipc_task.h — slim interceptor.
//
// The production header pulls in the entire IPC stack (task, queue, event,
// driver, sys/ipc/driver) via nv/ipc/common.h, which requires the
// platform-specific NV_IPC_CONFIG_H. Modules whose ESBMC harness only
// consumes nv::ipc::get_current_core() / CoreId — e.g., sys::c2c_mailbox —
// don't need that machinery, so we intercept the include here and surface
// only what's required.
//
// If a future harness needs more from ipc_task.h (Task class, Config, etc.),
// extend this stub rather than removing it; the heavy production chain is
// not buildable under ESBMC without a full NV_IPC_CONFIG_H.
#pragma once
#include "sys/common/common.h"
