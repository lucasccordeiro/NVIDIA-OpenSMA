ESBMC      ?= /Users/user/esbmc/build/src/esbmc/esbmc
REPO       := $(abspath $(CURDIR)/..)
STUBS      := $(CURDIR)/stubs
RESULTS    := $(CURDIR)/results
HARNESSES  := $(CURDIR)/harnesses

# Common include order: stubs first (so libc++ shims shadow real headers
# and pdk-cmn-flowcontrol.h is intercepted), then mctp-cpp src, then the
# x86 platform that provides PrivateHeader and the get/set helpers.
MCTP_INCS := \
  -I$(STUBS) \
  -I$(REPO)/corepdk/modules/mctp-cpp/src \
  -I$(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86

# Standard language-level safety profile. ESBMC enables pointer-check,
# bounds-check, and div-by-zero by default; we add the explicit opt-ins
# (memory-leak, overflow, unsigned overflow, NaN).
LANG_FLAGS := \
  --std c++20 \
  --memory-leak-check \
  --overflow-check \
  --unsigned-overflow-check \
  --nan-check \
  --unwind 4

# utils.h::align_to uses `value + alignment - 1` (left-to-right), so
# `value + alignment` can overflow for value = alignment = 2^31 even though
# the guard above passes. The wrap is benign in unsigned arithmetic — the
# final masked result is correct (finding F-2, retracted). Drop
# --unsigned-overflow-check here; utils_neg documents the false-positive.
UTILS_LANG_FLAGS := \
  --std c++20 \
  --memory-leak-check \
  --overflow-check \
  --ub-shift-check \
  --nan-check \
  --unwind 4

# Volatile-check profile: LANG_FLAGS + --volatile-check.
# Kept as a separate phase so volatile findings do not block the Phase 1 CI gate.
VOLATILE_FLAGS := \
  --std c++20 \
  --memory-leak-check \
  --overflow-check \
  --unsigned-overflow-check \
  --nan-check \
  --volatile-check \
  --unwind 4

UTILS_VOLATILE_FLAGS := \
  --std c++20 \
  --memory-leak-check \
  --overflow-check \
  --ub-shift-check \
  --nan-check \
  --volatile-check \
  --unwind 4

# Functional / inductive profile. Restored to k-induction with
# --interval-analysis after esbmc#4184 cleared the qualified-constexpr
# blocker. Works on all current targets because the verification-only
# overlay for pdk-mctp-app-packet.h replaces std::bit_cast<T*> with a
# C-cast (tagged WORKAROUND esbmc#4180 part 1) — that delta is what keeps
# the memcpy out of the inductive proof obligation.
FUNC_FLAGS := \
  --std c++20 \
  --overflow-check \
  --interval-analysis \
  --k-induction --k-step 1 --max-k-step 6

FP_INCS := -I$(REPO)/src

.PHONY: all volatile \
        mctp_packet mctp_packet_func mctp_packet_neg mctp_packet_volatile \
        mctp_router mctp_router_func mctp_router_neg mctp_router_volatile \
        mctp_validator mctp_validator_func mctp_validator_volatile \
        mctp_dispatch \
        nsm_type_2 nsm_type_2_func nsm_type_2_volatile \
        nsm_type3 nsm_type3_func nsm_type3_volatile \
        nsm_bitmask nsm_bitmask_func nsm_bitmask_neg nsm_bitmask_volatile \
        nsm_type5_validate nsm_type5_validate_func nsm_type5_validate_volatile \
        nsm_type5_f6_neg nsm_type5_f7_neg nsm_type5_f8_neg \
        telemetry telemetry_func telemetry_volatile \
        spi_utils spi_utils_func spi_utils_volatile \
        i2c_crc8 i2c_crc8_func i2c_crc8_volatile \
        literals literals_func literals_neg literals_volatile \
        fixed_point fixed_point_func fixed_point_volatile \
        utils utils_func utils_neg utils_volatile \
        ntc_table ntc_table_func ntc_table_volatile \
        nsm_type3_f10_neg \
        pwr_smooth_params pwr_smooth_params_func pwr_smooth_params_volatile \
        fru_utils fru_utils_func fru_utils_volatile \
        soc_sma_filter soc_sma_filter_func soc_sma_filter_volatile \
        debug_telemetry_sma debug_telemetry_sma_func debug_telemetry_sma_volatile \
        debug_telemetry_f13_neg \
        pca9555 pca9555_func pca9555_volatile \
        pca9555_f14_neg \
        emc1812 emc1812_func emc1812_volatile \
        tmp1075 tmp1075_func tmp1075_volatile \
        tmp461 tmp461_func tmp461_volatile \
        nsm_f6_system \
        nsm_f7_system \
        nsm_dcd_event_handlers \
        nsm_event_source_f15_neg \
        nsm_event_ack_f16_neg \
        nsm_gpio_safety \
        c2c_mailbox c2c_mailbox_volatile \
        clean
all: mctp_packet mctp_router mctp_validator nsm_type_2 nsm_type3 nsm_bitmask nsm_type5_validate telemetry spi_utils i2c_crc8 literals fixed_point utils ntc_table pwr_smooth_params fru_utils soc_sma_filter debug_telemetry_sma pca9555 emc1812 tmp1075 tmp461 c2c_mailbox
volatile: mctp_packet_volatile mctp_router_volatile mctp_validator_volatile nsm_type_2_volatile nsm_type3_volatile nsm_bitmask_volatile nsm_type5_validate_volatile telemetry_volatile spi_utils_volatile i2c_crc8_volatile literals_volatile fixed_point_volatile utils_volatile ntc_table_volatile pwr_smooth_params_volatile fru_utils_volatile soc_sma_filter_volatile debug_telemetry_sma_volatile pca9555_volatile emc1812_volatile tmp1075_volatile tmp461_volatile c2c_mailbox_volatile

# Phase 1: language-level checks on the MCTP packet parser.
mctp_packet:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) $(MCTP_INCS) \
	  $(HARNESSES)/mctp_packet_harness.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
	  2>&1 | tee $(RESULTS)/mctp_packet.log

# Phase 2: functional contract (round-trip equivalence) via k-induction.
mctp_packet_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) $(MCTP_INCS) \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/mctp_packet_harness.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
	  2>&1 | tee $(RESULTS)/mctp_packet_func.log

mctp_packet_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) $(MCTP_INCS) \
	  $(HARNESSES)/mctp_packet_negative_harness.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
	  2>&1 | tee $(RESULTS)/mctp_packet_neg.log

mctp_router_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) $(MCTP_INCS) \
	  $(HARNESSES)/mctp_router_negative_harness.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
	  2>&1 | tee $(RESULTS)/mctp_router_neg.log

mctp_router:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) $(MCTP_INCS) \
	  $(HARNESSES)/mctp_router_harness.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
	  2>&1 | tee $(RESULTS)/mctp_router.log

mctp_router_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) $(MCTP_INCS) \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/mctp_router_harness.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
	  2>&1 | tee $(RESULTS)/mctp_router_func.log

mctp_validator:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) $(MCTP_INCS) \
	  $(HARNESSES)/mctp_validator_harness.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/app/pdk-mctp-app-validator.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
	  2>&1 | tee $(RESULTS)/mctp_validator.log

mctp_validator_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --overflow-check --interval-analysis \
	  --k-induction --k-step 1 --max-k-step 16 \
	  $(MCTP_INCS) \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/mctp_validator_harness.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/app/pdk-mctp-app-validator.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
	  2>&1 | tee $(RESULTS)/mctp_validator_func.log

# F-1 reachability: Control SetEpId Request with gap interface [UsEnd=2, End=18)
# passes Validator::validate(), then on_set_endpoint_id() → set_cur_eid() OOBs.
# Note: Control ctrl{} triggers an ESBMC frontend crash (filed separately), so
# the harness calls set_cur_eid() directly after validate() returns true.
# Expected: VERIFICATION FAILED ("std::array::at out of range").
mctp_dispatch:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) $(MCTP_INCS) \
	  $(HARNESSES)/mctp_dispatch_harness.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/app/pdk-mctp-app-validator.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-control.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
	  2>&1 | tee $(RESULTS)/mctp_dispatch.log

# pciLinkValidDevices[11] → loop iterates 11 times; need unwind 12 to close.
nsm_type_2:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --unwind 12 \
	  $(HARNESSES)/nsm_type_2_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type_2.log

nsm_type_2_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --overflow-check --interval-analysis \
	  --k-induction --k-step 1 --max-k-step 16 \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/nsm_type_2_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type_2_func.log

# Phase 1+2: telemetry sensor-ID lookup functions + buffer_to_uint32 (fixed).
# Uses --ub-shift-check (not --unsigned-overflow-check): uint32_t<<24 through a
# function-call boundary triggers a false positive in ESBMC's unsigned-overflow
# model (loses the uint8_t range constraint after the cast).
# Note: the production buffer_to_uint32 has a misplaced cast (F-6, retracted):
#   static_cast<uint32_t>(buffer[3] << 24)  vs  static_cast<uint32_t>(buffer[3]) << 24
# C++20 [expr.shift]/2 makes signed left-shift wrapping well-defined for all
# inputs (same as the retracted F-3), so this is a style issue, not a bug.
# TempSensorIdToTelemIdMapping[10] → loop iterates 10 times; need unwind 11.
# (Also uses --ub-shift-check instead of --unsigned-overflow-check; see comment above.)
telemetry:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 \
	  --memory-leak-check \
	  --overflow-check \
	  --ub-shift-check \
	  --nan-check \
	  --unwind 11 \
	  $(HARNESSES)/telemetry_harness.cpp \
	  2>&1 | tee $(RESULTS)/telemetry.log

telemetry_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --overflow-check --interval-analysis \
	  --k-induction --k-step 1 --max-k-step 16 \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/telemetry_harness.cpp \
	  2>&1 | tee $(RESULTS)/telemetry_func.log

# Phase 1+2: NSM Type-3 sensor availability functions (inlined verbatim,
# testrunner platform config: 8 temp sensors, 3 power sensors, 0 voltage sensors).
# mcuTemperatureSensors[8] → loop iterates 8 times; need unwind 9 to close.
nsm_type3:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --unwind 9 \
	  $(HARNESSES)/nsm_type3_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type3.log

nsm_type3_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --overflow-check --interval-analysis \
	  --k-induction --k-step 1 --max-k-step 16 \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/nsm_type3_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type3_func.log

# Harness fills BufN=8 arrays; init loops iterate 8 times → unwind 9.
spi_utils:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --unwind 9 -I$(STUBS) \
	  $(HARNESSES)/spi_utils_harness.cpp \
	  2>&1 | tee $(RESULTS)/spi_utils.log

spi_utils_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --overflow-check --interval-analysis \
	  --k-induction --k-step 1 --max-k-step 12 \
	  -I$(STUBS) -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/spi_utils_harness.cpp \
	  2>&1 | tee $(RESULTS)/spi_utils_func.log

# LookupTable is static const std::array<uint8_t,256>; ESBMC models the
# aggregate initialiser as a loop → needs unwind 257 to prove exhaustion.
i2c_crc8:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --unwind 257 -I$(STUBS) \
	  $(HARNESSES)/i2c_crc8_harness.cpp \
	  2>&1 | tee $(RESULTS)/i2c_crc8.log

i2c_crc8_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --overflow-check --interval-analysis \
	  --k-induction --k-step 1 --max-k-step 12 \
	  -I$(STUBS) -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/i2c_crc8_harness.cpp \
	  2>&1 | tee $(RESULTS)/i2c_crc8_func.log

literals:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  $(HARNESSES)/literals_harness.cpp \
	  2>&1 | tee $(RESULTS)/literals.log

literals_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/literals_harness.cpp \
	  2>&1 | tee $(RESULTS)/literals_func.log

literals_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --ub-shift-check \
	  $(HARNESSES)/literals_negative_harness.cpp \
	  2>&1 | tee $(RESULTS)/literals_neg.log

fixed_point:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) $(FP_INCS) \
	  $(HARNESSES)/fixed_point_harness.cpp \
	  2>&1 | tee $(RESULTS)/fixed_point.log

fixed_point_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) $(FP_INCS) \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/fixed_point_harness.cpp \
	  2>&1 | tee $(RESULTS)/fixed_point_func.log

utils:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(UTILS_LANG_FLAGS) $(FP_INCS) -I$(STUBS) \
	  $(HARNESSES)/utils_harness.cpp \
	  2>&1 | tee $(RESULTS)/utils.log

utils_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) $(FP_INCS) -I$(STUBS) \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/utils_harness.cpp \
	  2>&1 | tee $(RESULTS)/utils_func.log

utils_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  $(HARNESSES)/utils_negative_harness.cpp \
	  2>&1 | tee $(RESULTS)/utils_neg.log

# Phase 1+2: nsm_msg bitmask operations (set_bit, unset_bit, get_bit, is_bit_set)
# on the 32-element (NvMctpSupportedNum) array. Direct include — no stubs needed.
nsm_bitmask:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) -I$(REPO)/src \
	  $(HARNESSES)/nsm_bitmask_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_bitmask.log

nsm_bitmask_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) -I$(REPO)/src \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/nsm_bitmask_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_bitmask_func.log

# Negative: set_bit on the 8-element (NvMctpEventSupportedNum) array with
# nondet pos ≥ 64 — byte_index ≥ 8 is OOB. Expected: VERIFICATION FAILED.
nsm_bitmask_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) -I$(REPO)/src \
	  $(HARNESSES)/nsm_bitmask_negative_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_bitmask_neg.log

# Phase 1+2: nsm_type_5 field validators (inlined verbatim, no MCU stubs needed).
nsm_type5_validate:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  $(HARNESSES)/nsm_type5_validate_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type5_validate.log

nsm_type5_validate_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/nsm_type5_validate_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type5_validate_func.log

# Phase 1+2: NTC thermistor lookup table and conversion functions.
# Binary search on 166 entries converges in ≤8 iterations → --unwind 9.
# Production ntc_table.cpp has no MCU dependencies; compiled directly.
ntc_table:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 \
	  --memory-leak-check \
	  --overflow-check \
	  --unsigned-overflow-check \
	  --nan-check \
	  --unwind 9 \
	  -I$(REPO)/src \
	  $(REPO)/src/nv/volt_mon/ntc_table.cpp \
	  $(HARNESSES)/ntc_table_harness.cpp \
	  2>&1 | tee $(RESULTS)/ntc_table.log

ntc_table_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 \
	  --overflow-check \
	  --interval-analysis \
	  --k-induction --k-step 1 --max-k-step 12 \
	  -DESBMC_FUNCTIONAL=1 \
	  -I$(REPO)/src \
	  $(REPO)/src/nv/volt_mon/ntc_table.cpp \
	  $(HARNESSES)/ntc_table_harness.cpp \
	  2>&1 | tee $(RESULTS)/ntc_table_func.log

# Phase 1+2: OverrideParam wire-format round-trips + is_valid_param_id characterisation.
# All logic inlined — no production source file needed (no MCU deps).
pwr_smooth_params:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  $(HARNESSES)/pwr_smooth_params_harness.cpp \
	  2>&1 | tee $(RESULTS)/pwr_smooth_params.log

pwr_smooth_params_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/pwr_smooth_params_harness.cpp \
	  2>&1 | tee $(RESULTS)/pwr_smooth_params_func.log

# Phase 1+2: FRU verify_checksum + decode_6bit_ascii (inlined — no I2C/logger deps).
# verify_checksum iterates over buf[8]; need unwind 9 to close.
fru_utils:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --unwind 9 \
	  $(HARNESSES)/fru_utils_harness.cpp \
	  2>&1 | tee $(RESULTS)/fru_utils.log

fru_utils_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 \
	  --overflow-check \
	  --interval-analysis \
	  --k-induction --k-step 1 --max-k-step 10 \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/fru_utils_harness.cpp \
	  2>&1 | tee $(RESULTS)/fru_utils_func.log

# Phase 1+2: 4-sample SMA filter for SoC% telemetry (SocSmaFilterCh).
# Pure SFXP22_10 arithmetic; no MCU platform includes.
soc_sma_filter:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) -I$(STUBS) -I$(REPO)/src \
	  $(HARNESSES)/soc_sma_filter_harness.cpp \
	  2>&1 | tee $(RESULTS)/soc_sma_filter.log

soc_sma_filter_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) -I$(STUBS) -I$(REPO)/src \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/soc_sma_filter_harness.cpp \
	  2>&1 | tee $(RESULTS)/soc_sma_filter_func.log

# Phase 1+2: 256-sample SMA for debug telemetry (DebugTelemetrySmaCh).
# Mixed-type arithmetic: SFXP22_10 → SFXP32_0 → UFXP8_0 buffer.
debug_telemetry_sma:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) -I$(STUBS) -I$(REPO)/src \
	  $(HARNESSES)/debug_telemetry_sma_harness.cpp \
	  2>&1 | tee $(RESULTS)/debug_telemetry_sma.log

debug_telemetry_sma_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) -I$(STUBS) -I$(REPO)/src \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/debug_telemetry_sma_harness.cpp \
	  2>&1 | tee $(RESULTS)/debug_telemetry_sma_func.log

# Phase 1+2: PCA9555 16-bit I2C GPIO expander emulator (Pca9555).
# Pure bit-manipulation state machine; only dep is sys/i2c/i2c_slave.h
# (for I2cSlaveBufferSize) and nv/nv.h (logging, no-op stubbed).
# Both stubs live under stubs/sys/i2c/ and stubs/nv/ respectively.
pca9555:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/sys/i2c -I$(REPO)/src \
	  $(HARNESSES)/pca9555_harness.cpp \
	  $(REPO)/src/nv/emulation/pca9555.cpp \
	  2>&1 | tee $(RESULTS)/pca9555.log

pca9555_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/sys/i2c -I$(REPO)/src \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/pca9555_harness.cpp \
	  $(REPO)/src/nv/emulation/pca9555.cpp \
	  2>&1 | tee $(RESULTS)/pca9555_func.log

# Phase 1+2: EMC1812 temperature sensor driver (Emc1812).
# Verifies signed/unsigned register cast round-trip and safety.
# TempSensor base class stubbed via stubs/nv/i2c/sensor.h (nondet I2C).
emc1812:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/nv/i2c -I$(REPO)/src \
	  $(HARNESSES)/emc1812_harness.cpp \
	  $(REPO)/src/nv/i2c/emc1812.cpp \
	  2>&1 | tee $(RESULTS)/emc1812.log

emc1812_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/nv/i2c -I$(REPO)/src \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/emc1812_harness.cpp \
	  $(REPO)/src/nv/i2c/emc1812.cpp \
	  2>&1 | tee $(RESULTS)/emc1812_func.log

# Phase 1+2: TMP1075 temperature sensor driver.
# Verifies 12-bit encoding round-trip: int8_t → <<4 → uint16_t → >>4 → int8_t.
# TempSensor base class stubbed via stubs/nv/i2c/sensor.h (nondet I2C).
tmp1075:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/nv/i2c -I$(REPO)/src \
	  $(HARNESSES)/tmp1075_harness.cpp \
	  $(REPO)/src/nv/i2c/tmp1075.cpp \
	  2>&1 | tee $(RESULTS)/tmp1075.log

tmp1075_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/nv/i2c -I$(REPO)/src \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/tmp1075_harness.cpp \
	  $(REPO)/src/nv/i2c/tmp1075.cpp \
	  2>&1 | tee $(RESULTS)/tmp1075_func.log

# Phase 1+2: TMP461 (/ NCT72) temperature sensor driver.
# Verifies int8_t↔uint8_t threshold cast round-trip (same pattern as emc1812).
# TempSensor base class stubbed via stubs/nv/i2c/sensor.h (nondet I2C).
tmp461:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/nv/i2c -I$(REPO)/src \
	  $(HARNESSES)/tmp461_harness.cpp \
	  $(REPO)/src/nv/i2c/tmp461.cpp \
	  2>&1 | tee $(RESULTS)/tmp461.log

tmp461_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(FUNC_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/nv/i2c -I$(REPO)/src \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/tmp461_harness.cpp \
	  $(REPO)/src/nv/i2c/tmp461.cpp \
	  2>&1 | tee $(RESULTS)/tmp461_func.log

# =============================================================================
# Security findings (negative targets) — expected: VERIFICATION FAILED.
# These are not part of 'all' or 'volatile'; run individually or as a group.
# Each harness is self-contained (no production source files needed).
# =============================================================================

# F-6: SetErrorInjectionMode stores nrx.data[0] without a range check, accepting
# any uint8_t mode value instead of only Disable (0x00) or Enable (0x01).
nsm_type5_f6_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  $(HARNESSES)/nsm_type5_f6_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type5_f6_neg.log

# F-7: on_dev_cfg_submit_ErrorInjectionPayload writes portRecoveryResp before
# validating; on failure it returns without resetting the field (no rollback),
# unlike the FatalError and USBBridgeEmulation paths which zero the field first.
nsm_type5_f7_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  $(HARNESSES)/nsm_type5_f7_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type5_f7_neg.log

# F-8: validateGpioSpoofingErrorInjectionPayload iterates up to ei_gpio_number
# with no bounds check; gpio_ei_entries has 16 slots. OOB access at
# gpio_ei_entries[i] for i >= 16. The caller guards this but the validator itself
# does not, so any direct call with ei_gpio_number > 16 is unsafe.
# Fill and validator loops both iterate ≤17 times → unwind 17.
nsm_type5_f8_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --unwind 17 \
	  $(HARNESSES)/nsm_type5_f8_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type5_f8_neg.log

# F-10: set_busbar_temperature_threshold silently substitutes 125°C when
# ntc_temperature_to_resistance returns 0 (out-of-range temperature), then
# returns Ccode::Success instead of an error code.
nsm_type3_f10_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) $(FP_INCS) \
	  $(REPO)/src/nv/volt_mon/ntc_table.cpp \
	  $(HARNESSES)/nsm_type3_f10_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type3_f10_neg.log

# F-13: DebugTelemetrySmaCh::evaluate() casts SFXP32_0 to UFXP8_0 (uint8_t)
# without checking for negative values; a negative percent wraps modulo 256,
# corrupting the SMA buffer (e.g., -1% → sfxp32_0 = -1 → uint8_t = 255).
debug_telemetry_f13_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) -I$(STUBS) -I$(REPO)/src \
	  $(HARNESSES)/debug_telemetry_f13_harness.cpp \
	  2>&1 | tee $(RESULTS)/debug_telemetry_f13_neg.log

# F-13 system-level: PowerManager::run_iteration() with nondet ADC + nondet GPIO.
# Verifies that DebugTelemetrySmaCh receives only non-negative inputs under all
# hardware states — upstream std::clamp in StateOfChargeDev::soc_voltage_to_percent()
# and OffsetPolicy clamps all three channels to [0%, 100%].
# Stubs: sys/adc (nondet pop_fifo_conv_value), sys/dac (no-op set),
#        sys/gpio (nondet read for thermal-warn), power_manager_config.h.
# Expected: VERIFICATION SUCCESSFUL — F-13 is a latent defect unreachable from
# current production data flow; the function is unsafe but the system is safe.
debug_telemetry_f13_system:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/sys/adc -I$(STUBS)/sys/dac -I$(STUBS)/sys/gpio \
	  -I$(REPO)/src \
	  -DNV_IPC_CONFIG_H=\"power_manager_config.h\" \
	  $(HARNESSES)/debug_telemetry_f13_system_harness.cpp \
	  2>&1 | tee $(RESULTS)/debug_telemetry_f13_system.log

# F-14: Pca9555::i2c_write bare `return` on Input command — production code
# confirmation that the write leaves output state unchanged (VERIFICATION SUCCESSFUL).
# CommandRegister is fixed per call (cmd_byte/2); Input case has no body; return and
# break are observably equivalent. F-14 is a code-quality note, not a security finding.
pca9555_f14_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/sys/i2c -I$(REPO)/src \
	  $(REPO)/src/nv/emulation/pca9555.cpp \
	  $(HARNESSES)/pca9555_f14_harness.cpp \
	  2>&1 | tee $(RESULTS)/pca9555_f14_neg.log

# =============================================================================
# Phase: volatile — all Phase 1 targets with --volatile-check added.
# Run with: make volatile
# Separate from Phase 1 so volatile findings do not block the CI gate.
# =============================================================================

mctp_packet_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) $(MCTP_INCS) \
	  $(HARNESSES)/mctp_packet_harness.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
	  2>&1 | tee $(RESULTS)/mctp_packet_volatile.log

mctp_router_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) $(MCTP_INCS) \
	  $(HARNESSES)/mctp_router_harness.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
	  2>&1 | tee $(RESULTS)/mctp_router_volatile.log

mctp_validator_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) $(MCTP_INCS) \
	  $(HARNESSES)/mctp_validator_harness.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/app/pdk-mctp-app-validator.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
	  $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp \
	  2>&1 | tee $(RESULTS)/mctp_validator_volatile.log

nsm_type_2_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --volatile-check --unwind 12 \
	  $(HARNESSES)/nsm_type_2_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type_2_volatile.log

nsm_type3_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --volatile-check --unwind 9 \
	  $(HARNESSES)/nsm_type3_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type3_volatile.log

nsm_bitmask_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) -I$(REPO)/src \
	  $(HARNESSES)/nsm_bitmask_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_bitmask_volatile.log

nsm_type5_validate_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) \
	  $(HARNESSES)/nsm_type5_validate_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type5_validate_volatile.log

telemetry_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 \
	  --memory-leak-check \
	  --overflow-check \
	  --ub-shift-check \
	  --nan-check \
	  --volatile-check \
	  --unwind 11 \
	  $(HARNESSES)/telemetry_harness.cpp \
	  2>&1 | tee $(RESULTS)/telemetry_volatile.log

spi_utils_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --volatile-check --unwind 9 -I$(STUBS) \
	  $(HARNESSES)/spi_utils_harness.cpp \
	  2>&1 | tee $(RESULTS)/spi_utils_volatile.log

i2c_crc8_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --volatile-check --unwind 257 -I$(STUBS) \
	  $(HARNESSES)/i2c_crc8_harness.cpp \
	  2>&1 | tee $(RESULTS)/i2c_crc8_volatile.log

literals_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) \
	  $(HARNESSES)/literals_harness.cpp \
	  2>&1 | tee $(RESULTS)/literals_volatile.log

fixed_point_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) $(FP_INCS) \
	  $(HARNESSES)/fixed_point_harness.cpp \
	  2>&1 | tee $(RESULTS)/fixed_point_volatile.log

utils_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(UTILS_VOLATILE_FLAGS) $(FP_INCS) -I$(STUBS) \
	  $(HARNESSES)/utils_harness.cpp \
	  2>&1 | tee $(RESULTS)/utils_volatile.log

ntc_table_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 \
	  --memory-leak-check \
	  --overflow-check \
	  --unsigned-overflow-check \
	  --nan-check \
	  --volatile-check \
	  --unwind 9 \
	  -I$(REPO)/src \
	  $(REPO)/src/nv/volt_mon/ntc_table.cpp \
	  $(HARNESSES)/ntc_table_harness.cpp \
	  2>&1 | tee $(RESULTS)/ntc_table_volatile.log

pwr_smooth_params_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) \
	  $(HARNESSES)/pwr_smooth_params_harness.cpp \
	  2>&1 | tee $(RESULTS)/pwr_smooth_params_volatile.log

fru_utils_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --volatile-check --unwind 9 \
	  $(HARNESSES)/fru_utils_harness.cpp \
	  2>&1 | tee $(RESULTS)/fru_utils_volatile.log

soc_sma_filter_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) -I$(STUBS) -I$(REPO)/src \
	  $(HARNESSES)/soc_sma_filter_harness.cpp \
	  2>&1 | tee $(RESULTS)/soc_sma_filter_volatile.log

debug_telemetry_sma_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) -I$(STUBS) -I$(REPO)/src \
	  $(HARNESSES)/debug_telemetry_sma_harness.cpp \
	  2>&1 | tee $(RESULTS)/debug_telemetry_sma_volatile.log

pca9555_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/sys/i2c -I$(REPO)/src \
	  $(HARNESSES)/pca9555_harness.cpp \
	  $(REPO)/src/nv/emulation/pca9555.cpp \
	  2>&1 | tee $(RESULTS)/pca9555_volatile.log

emc1812_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/nv/i2c -I$(REPO)/src \
	  $(HARNESSES)/emc1812_harness.cpp \
	  $(REPO)/src/nv/i2c/emc1812.cpp \
	  2>&1 | tee $(RESULTS)/emc1812_volatile.log

tmp1075_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/nv/i2c -I$(REPO)/src \
	  $(HARNESSES)/tmp1075_harness.cpp \
	  $(REPO)/src/nv/i2c/tmp1075.cpp \
	  2>&1 | tee $(RESULTS)/tmp1075_volatile.log

tmp461_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) \
	  -I$(STUBS) -I$(STUBS)/nv/i2c -I$(REPO)/src \
	  $(HARNESSES)/tmp461_harness.cpp \
	  $(REPO)/src/nv/i2c/tmp461.cpp \
	  2>&1 | tee $(RESULTS)/tmp461_volatile.log

# F-6 system-level reachability proof: Nsm::process_device_configuration() with
# nondet nrx.data[0] compiled from the real nsm_type_5.cpp production source.
# Stubs intercept all IPC/FreeRTOS/flash/logger dependencies; only the F-6
# code path (process_device_configuration → on_dev_cfg_set_errorInjectionMode)
# is verified. Expected: VERIFICATION FAILED — mode stored without range check.
# count_events_in_bitmask iterates NvMctpEventSupportedNum=8 times; outer for
# and inner while each need at most 8 iterations → --unwind 9 to close both.
nsm_f6_system:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --unwind 9 \
	  -I$(STUBS) \
	  -I$(REPO) \
	  -I$(REPO)/src \
	  -I$(REPO)/corepdk/modules/mctp-cpp/src \
	  -I$(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86 \
	  -DNV_IPC_CONFIG_H=\"nsm_f6_config.h\" \
	  $(HARNESSES)/nsm_f6_system_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_f6_system.log

# F-7 system-level harness: Nsm::process_device_configuration() compiled from
# production nsm_type_5.cpp with a SetErrorInjectionPayload/PortRecoveryErrors
# request (OCP v2, DeviceError id, nondet bitmaps).  Closes gap-1 (real types)
# and gap-2 (real dispatch logic); gap-3 (validator as nondet) remains because
# validatePortRecoveryErrorInjectionPayload always returns true (TODO stub).
# count_events_in_bitmask iterates 8 times; production memcpy copies
# sizeof(PortRecoveryPayload)=12 bytes — 12 iterations dominates → --unwind 13.
# Expected: VERIFICATION SUCCESSFUL — validator never returns false, so the
# write-before-validate no-rollback path is dead code in current production code.
nsm_f7_system:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --unwind 13 \
	  -I$(STUBS) \
	  -I$(REPO) \
	  -I$(REPO)/src \
	  -I$(REPO)/corepdk/modules/mctp-cpp/src \
	  -I$(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86 \
	  -DNV_IPC_CONFIG_H=\"nsm_f6_config.h\" \
	  $(HARNESSES)/nsm_f7_system_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_f7_system.log

# DCD event-bitmask write handlers: on_dcd_set_current_event_srcs (nsm.cpp:983)
# and on_dcd_configure_event_ack (nsm.cpp:1096). Both AND-down a packet-supplied
# 8-byte bitmask with the per-type Sup*Event mask under a for-loop bounded by
# NvMctpEventSupportedNum=8 → --unwind 9. Sibling check to F-15/F-16 (read-side
# OOBs): confirms the write-side gates the .at(uint8_t(nv_msg_type)) index on
# nv_msg_type ∈ {DCD=0, Firmware=6}, both within log_nvmsg_event_bitmask.size()=32.
# Expected: VERIFICATION SUCCESSFUL.
nsm_dcd_event_handlers:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --unwind 10 \
	  -I$(STUBS) \
	  -I$(REPO) \
	  -I$(REPO)/src \
	  -I$(REPO)/corepdk/modules/mctp-cpp/src \
	  -I$(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86 \
	  $(HARNESSES)/nsm_dcd_event_handlers_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_dcd_event_handlers.log

# F-15: is_event_source_enable reads type0/6_event_enable_bitmask.at(event_id/8)
# without a bounds guard. For event_id ∈ [64, 255], ByteIndex ∈ [8, 31] is OOB
# on a size-8 array (NvMctpEventSupportedNum=8). No loop → default --unwind 4 ok.
# Expected: VERIFICATION FAILED ("Index out of bounds") on either msg_type branch.
nsm_event_source_f15_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) -I$(REPO)/src \
	  $(HARNESSES)/nsm_event_source_f15_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_event_source_f15_neg.log

# F-16: is_event_ack_enable (nsm.cpp:1089) computes ByteIndex = event_id/8 and
# calls type0/6_event_ack_bitmask.at(ByteIndex) without a bounds check — same
# asymmetric-guard pattern as F-5 and F-15. For event_id ∈ [64,255], ByteIndex
# ∈ [8,31] is OOB on a size-8 array. No loop → default --unwind 4 ok.
# Expected: VERIFICATION FAILED ("Index out of bounds") on either msg_type branch.
nsm_event_ack_f16_neg:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) -I$(REPO)/src \
	  $(HARNESSES)/nsm_event_ack_f16_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_event_ack_f16_neg.log

# DCD GPIO safety proof: on_dcd_get_gpio / on_dcd_set_gpio structural harness.
# GpioNum = 66 (p3957_cxx: 8+11+10+18+11+8); main loop iterates ≤ 66 times.
# Invariant: (offset + length) ≤ GpioNum ensures all .at() calls are in bounds.
# Expected: VERIFICATION SUCCESSFUL.
nsm_gpio_safety:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --unwind 67 \
	  $(HARNESSES)/nsm_gpio_safety_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_gpio_safety.log

# sys::c2c_mailbox — Phase 1 safety baseline.
# No loops, no memcpy → default --unwind 4 is sufficient.
# Stubs: fsl_mailbox.h (NXP SDK shim) + nv/ipc/ipc_task.h (slim interceptor
# that surfaces only get_current_core / CoreId via sys/common/common.h).
# Expected: VERIFICATION SUCCESSFUL
c2c_mailbox:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  -I$(STUBS) -I$(REPO)/src -I$(REPO)/src/sys/mcxn556 \
	  $(HARNESSES)/c2c_mailbox_harness.cpp \
	  $(REPO)/src/sys/mcxn556/sys/c2c_mailbox/c2c_mailbox.cpp \
	  2>&1 | tee $(RESULTS)/c2c_mailbox.log

c2c_mailbox_volatile:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(VOLATILE_FLAGS) \
	  -I$(STUBS) -I$(REPO)/src -I$(REPO)/src/sys/mcxn556 \
	  $(HARNESSES)/c2c_mailbox_harness.cpp \
	  $(REPO)/src/sys/mcxn556/sys/c2c_mailbox/c2c_mailbox.cpp \
	  2>&1 | tee $(RESULTS)/c2c_mailbox_volatile.log

clean:
	rm -rf $(RESULTS)

# ----------------------------------------------------------------------------
# Phase 4 — coverage of structurally-verified modules.
#
# Each in-scope module gets two coverage rules: <target>_cov_p1 (Phase 1,
# language-level safety profile) and <target>_cov_p2 (Phase 2, k-induction).
# scripts/cov_run.sh tries --k-path-coverage first; if ESBMC reports zero
# k-path goals (flat harness CFG, deeply inlined header code), it falls
# back to --branch-function-coverage. Outputs go under results/cov/, never
# results/, so a casual grep for VERIFICATION FAILED in the safety logs
# stays uncontaminated by coverage-mode "failures" (coverage runs activate
# --multi-property and surface unreached goals as failed claims).
#
# Aggregate targets: cov_p1, cov_p2, cov (= cov_p1 + cov_p2 + summarise).
# See verification/PHASE4_PLAN.md for the rubric and decision history.

COV_RUN := ./scripts/cov_run.sh
COV     := $(RESULTS)/cov

# nsm_dcd_event_handlers — no _func variant in Phase 1/2; the p2 rule reuses
# its custom flags+includes but swaps in the FUNC_FLAGS profile.
nsm_dcd_event_handlers_cov_p1:
	@mkdir -p $(COV)
	@$(COV_RUN) nsm_dcd_event_handlers p1 $(ESBMC) \
	  --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --unwind 10 \
	  -I$(STUBS) -I$(REPO) -I$(REPO)/src \
	  -I$(REPO)/corepdk/modules/mctp-cpp/src \
	  -I$(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86 \
	  -- $(HARNESSES)/nsm_dcd_event_handlers_harness.cpp

nsm_dcd_event_handlers_cov_p2:
	@mkdir -p $(COV)
	@$(COV_RUN) nsm_dcd_event_handlers p2 $(ESBMC) \
	  $(FUNC_FLAGS) \
	  -I$(STUBS) -I$(REPO) -I$(REPO)/src \
	  -I$(REPO)/corepdk/modules/mctp-cpp/src \
	  -I$(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86 \
	  -- $(HARNESSES)/nsm_dcd_event_handlers_harness.cpp

soc_sma_filter_cov_p1:
	@mkdir -p $(COV)
	@$(COV_RUN) soc_sma_filter p1 $(ESBMC) \
	  $(LANG_FLAGS) -I$(STUBS) -I$(REPO)/src \
	  -- $(HARNESSES)/soc_sma_filter_harness.cpp

soc_sma_filter_cov_p2:
	@mkdir -p $(COV)
	@$(COV_RUN) soc_sma_filter p2 $(ESBMC) \
	  $(FUNC_FLAGS) -DESBMC_FUNCTIONAL=1 \
	  -I$(STUBS) -I$(REPO)/src \
	  -- $(HARNESSES)/soc_sma_filter_harness.cpp

debug_telemetry_sma_cov_p1:
	@mkdir -p $(COV)
	@$(COV_RUN) debug_telemetry_sma p1 $(ESBMC) \
	  $(LANG_FLAGS) -I$(STUBS) -I$(REPO)/src \
	  -- $(HARNESSES)/debug_telemetry_sma_harness.cpp

debug_telemetry_sma_cov_p2:
	@mkdir -p $(COV)
	@$(COV_RUN) debug_telemetry_sma p2 $(ESBMC) \
	  $(FUNC_FLAGS) -DESBMC_FUNCTIONAL=1 \
	  -I$(STUBS) -I$(REPO)/src \
	  -- $(HARNESSES)/debug_telemetry_sma_harness.cpp

pca9555_cov_p1:
	@mkdir -p $(COV)
	@$(COV_RUN) pca9555 p1 $(ESBMC) \
	  $(LANG_FLAGS) -I$(STUBS) -I$(STUBS)/sys/i2c -I$(REPO)/src \
	  -- $(HARNESSES)/pca9555_harness.cpp \
	     $(REPO)/src/nv/emulation/pca9555.cpp

pca9555_cov_p2:
	@mkdir -p $(COV)
	@$(COV_RUN) pca9555 p2 $(ESBMC) \
	  $(FUNC_FLAGS) -DESBMC_FUNCTIONAL=1 \
	  -I$(STUBS) -I$(STUBS)/sys/i2c -I$(REPO)/src \
	  -- $(HARNESSES)/pca9555_harness.cpp \
	     $(REPO)/src/nv/emulation/pca9555.cpp

ntc_table_cov_p1:
	@mkdir -p $(COV)
	@$(COV_RUN) ntc_table p1 $(ESBMC) \
	  --std c++20 --memory-leak-check --overflow-check \
	  --unsigned-overflow-check --nan-check --unwind 9 \
	  -I$(REPO)/src \
	  -- $(REPO)/src/nv/volt_mon/ntc_table.cpp \
	     $(HARNESSES)/ntc_table_harness.cpp

ntc_table_cov_p2:
	@mkdir -p $(COV)
	@# --max-k-step 4 (vs 12 in ntc_table_func): the 166-entry binary-search
	@# VCC count multiplied by k-path-coverage's multi-property mode pushes
	@# the original k=12 setting past Phase 4's 5-minute per-run budget.
	@# k=4 still exercises every harness path; the inductive depth matters
	@# for the safety proof, not for path enumeration.
	@$(COV_RUN) ntc_table p2 $(ESBMC) \
	  --std c++20 --overflow-check --interval-analysis \
	  --k-induction --k-step 1 --max-k-step 4 \
	  -DESBMC_FUNCTIONAL=1 -I$(REPO)/src \
	  -- $(REPO)/src/nv/volt_mon/ntc_table.cpp \
	     $(HARNESSES)/ntc_table_harness.cpp

mctp_validator_cov_p1:
	@mkdir -p $(COV)
	@$(COV_RUN) mctp_validator p1 $(ESBMC) \
	  $(LANG_FLAGS) $(MCTP_INCS) \
	  -- $(HARNESSES)/mctp_validator_harness.cpp \
	     $(REPO)/corepdk/modules/mctp-cpp/src/app/pdk-mctp-app-validator.cpp \
	     $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
	     $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp

mctp_validator_cov_p2:
	@mkdir -p $(COV)
	@$(COV_RUN) mctp_validator p2 $(ESBMC) \
	  --std c++20 --overflow-check --interval-analysis \
	  --k-induction --k-step 1 --max-k-step 16 \
	  $(MCTP_INCS) -DESBMC_FUNCTIONAL=1 \
	  -- $(HARNESSES)/mctp_validator_harness.cpp \
	     $(REPO)/corepdk/modules/mctp-cpp/src/app/pdk-mctp-app-validator.cpp \
	     $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
	     $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp

nsm_bitmask_cov_p1:
	@mkdir -p $(COV)
	@$(COV_RUN) nsm_bitmask p1 $(ESBMC) \
	  $(LANG_FLAGS) -I$(REPO)/src \
	  -- $(HARNESSES)/nsm_bitmask_harness.cpp

nsm_bitmask_cov_p2:
	@mkdir -p $(COV)
	@$(COV_RUN) nsm_bitmask p2 $(ESBMC) \
	  $(FUNC_FLAGS) -DESBMC_FUNCTIONAL=1 -I$(REPO)/src \
	  -- $(HARNESSES)/nsm_bitmask_harness.cpp

mctp_packet_cov_p1:
	@mkdir -p $(COV)
	@$(COV_RUN) mctp_packet p1 $(ESBMC) \
	  $(LANG_FLAGS) $(MCTP_INCS) \
	  -- $(HARNESSES)/mctp_packet_harness.cpp \
	     $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp

mctp_packet_cov_p2:
	@mkdir -p $(COV)
	@$(COV_RUN) mctp_packet p2 $(ESBMC) \
	  $(FUNC_FLAGS) $(MCTP_INCS) -DESBMC_FUNCTIONAL=1 \
	  -- $(HARNESSES)/mctp_packet_harness.cpp \
	     $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp

mctp_router_cov_p1:
	@mkdir -p $(COV)
	@$(COV_RUN) mctp_router p1 $(ESBMC) \
	  $(LANG_FLAGS) $(MCTP_INCS) \
	  -- $(HARNESSES)/mctp_router_harness.cpp \
	     $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
	     $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp

mctp_router_cov_p2:
	@mkdir -p $(COV)
	@$(COV_RUN) mctp_router p2 $(ESBMC) \
	  $(FUNC_FLAGS) $(MCTP_INCS) -DESBMC_FUNCTIONAL=1 \
	  -- $(HARNESSES)/mctp_router_harness.cpp \
	     $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-router-plat.cpp \
	     $(REPO)/corepdk/modules/mctp-cpp/src/platforms/x86/pdk-mctp-platforms-packet-plat.cpp

nsm_type5_validate_cov_p1:
	@mkdir -p $(COV)
	@$(COV_RUN) nsm_type5_validate p1 $(ESBMC) \
	  $(LANG_FLAGS) \
	  -- $(HARNESSES)/nsm_type5_validate_harness.cpp

nsm_type5_validate_cov_p2:
	@mkdir -p $(COV)
	@$(COV_RUN) nsm_type5_validate p2 $(ESBMC) \
	  $(FUNC_FLAGS) -DESBMC_FUNCTIONAL=1 \
	  -- $(HARNESSES)/nsm_type5_validate_harness.cpp

COV_TARGETS := nsm_dcd_event_handlers soc_sma_filter debug_telemetry_sma \
               pca9555 ntc_table mctp_validator nsm_bitmask mctp_packet \
               mctp_router nsm_type5_validate

cov_p1: $(addsuffix _cov_p1, $(COV_TARGETS))
cov_p2: $(addsuffix _cov_p2, $(COV_TARGETS))

cov: cov_p1 cov_p2
	@python3 scripts/cov_aggregate.py $(COV) > $(COV)/cov_summary.tsv
	@python3 scripts/cov_aggregate.py --markdown $(COV) > $(COV)/cov_summary.md
	@echo "Phase 4 summary written to $(COV)/cov_summary.{tsv,md}"

.PHONY: cov cov_p1 cov_p2 \
        $(addsuffix _cov_p1, $(COV_TARGETS)) \
        $(addsuffix _cov_p2, $(COV_TARGETS))
