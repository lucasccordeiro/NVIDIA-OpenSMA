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
        nsm_type_2 nsm_type_2_func \
        spi_utils spi_utils_func \
        i2c_crc8 i2c_crc8_func \
        literals literals_func literals_neg \
        fixed_point fixed_point_func \
        utils utils_func utils_neg clean
all: mctp_packet mctp_router mctp_validator nsm_type_2 spi_utils i2c_crc8 literals fixed_point utils

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
	$(ESBMC) $(LANG_FLAGS) $(FP_INCS) -I$(STUBS) \
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

clean:
	rm -rf $(RESULTS)
