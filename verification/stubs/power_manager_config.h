// Minimal NV_IPC_CONFIG_H substitute for PowerManager ESBMC verification.
// Provides all hardware-platform constants used by devices.h, power_manager.h,
// and the policy headers.  Values match testrunner/config.h exactly.
// Must also bring in nv::common (offset_policy.h uses `using namespace nv::common`).
#pragma once
#include <cstdint>
#include "nv/common/utils.h"

constexpr bool     SocAdcHiResMode             = true;
constexpr uint32_t SocAdcPeripheral            = 0;
constexpr uint32_t SocAdcFifoNum               = 0;
constexpr uint32_t SocAdcInitialTriggerCommand = 1;
constexpr uint32_t EdppDacPeripheral           = 0;
constexpr uint32_t IsinkDacPeripheral          = 1;
constexpr uint32_t PwrBrakeGpioPort            = 1;   // nv::ipc::MCU_PWR_BRAKE_L_PORT
constexpr uint32_t PwrBrakeGpioPin             = 16;  // nv::ipc::MCU_PWR_BRAKE_L_PIN
constexpr uint32_t McuThermWarnPort            = 5;   // nv::ipc::MCU_THERM_WARN_L_PORT
constexpr uint32_t McuThermWarnPin             = 7;   // nv::ipc::MCU_THERM_WARN_L_PIN
constexpr float    OvrmMaxDacOutputV           = 2.2f;
constexpr float    SocAdcRefVoltageV           = 3.3f;
constexpr uint32_t SocVoltageDividerR1         = 10000;
constexpr uint32_t SocVoltageDividerR2         = 3320;
constexpr float    SocVoltageMin               = 1.0f;
constexpr float    SocVoltageMax               = 7.0f;
