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
  --unwind 4 \
  --no-unwinding-assertions

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
  --unwind 4 \
  --no-unwinding-assertions

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

.PHONY: all mctp_packet mctp_packet_func mctp_packet_neg \
        mctp_router mctp_router_func mctp_router_neg \
        mctp_validator mctp_validator_func \
        mctp_dispatch \
        nsm_type_2 nsm_type_2_func \
        nsm_type3 nsm_type3_func \
        nsm_bitmask nsm_bitmask_func nsm_bitmask_neg \
        nsm_type5_validate nsm_type5_validate_func \
        telemetry telemetry_func \
        spi_utils spi_utils_func \
        i2c_crc8 i2c_crc8_func \
        literals literals_func literals_neg \
        fixed_point fixed_point_func \
        utils utils_func utils_neg clean
all: mctp_packet mctp_router mctp_validator nsm_type_2 nsm_type3 nsm_bitmask nsm_type5_validate telemetry spi_utils i2c_crc8 literals fixed_point utils

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

nsm_type_2:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
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
# Uses --ub-shift-check (not --unsigned-overflow-check) because uint32_t<<24
# triggers a false positive in ESBMC's unsigned-overflow model for the fixed
# form; --ub-shift-check correctly accepts it.
# Note: the production buffer_to_uint32 has a misplaced cast (F-6, latent UB):
#   static_cast<uint32_t>(buffer[3] << 24)  should be
#   static_cast<uint32_t>(buffer[3]) << 24
# ESBMC cannot prove this because it models signed shifts as bitvector wrap
# (same limitation as the retracted F-3). The harness verifies the fixed form.
telemetry:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 \
	  --memory-leak-check \
	  --overflow-check \
	  --ub-shift-check \
	  --nan-check \
	  --unwind 4 \
	  --no-unwinding-assertions \
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
nsm_type3:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) \
	  $(HARNESSES)/nsm_type3_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type3.log

nsm_type3_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --overflow-check --interval-analysis \
	  --k-induction --k-step 1 --max-k-step 16 \
	  -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/nsm_type3_harness.cpp \
	  2>&1 | tee $(RESULTS)/nsm_type3_func.log

spi_utils:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) -I$(STUBS) \
	  $(HARNESSES)/spi_utils_harness.cpp \
	  2>&1 | tee $(RESULTS)/spi_utils.log

spi_utils_func:
	@mkdir -p $(RESULTS)
	$(ESBMC) --std c++20 --overflow-check --interval-analysis \
	  --k-induction --k-step 1 --max-k-step 12 \
	  -I$(STUBS) -DESBMC_FUNCTIONAL=1 \
	  $(HARNESSES)/spi_utils_harness.cpp \
	  2>&1 | tee $(RESULTS)/spi_utils_func.log

i2c_crc8:
	@mkdir -p $(RESULTS)
	$(ESBMC) $(LANG_FLAGS) -I$(STUBS) \
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

clean:
	rm -rf $(RESULTS)
