ESBMC      ?= /Users/user/esbmc/build/src/esbmc/esbmc
REPO       := $(abspath $(CURDIR)/..)
STUBS      := $(CURDIR)/stubs
RESULTS    := $(CURDIR)/results
HARNESSES  := $(CURDIR)/harnesses

# Include order: stubs first so they shadow real headers, then mctp-cpp src,
# then the x86 platform that provides PrivateHeader and get/set helpers.
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

# Functional / inductive profile.
FUNC_FLAGS := \
  --std c++20 \
  --overflow-check \
  --k-induction --k-step 1 --max-k-step 6

FP_INCS := -I$(REPO)/src

.PHONY: all mctp_packet mctp_packet_func mctp_packet_neg \
        mctp_router mctp_router_func mctp_router_neg \
        fixed_point fixed_point_func clean
all: mctp_packet mctp_router fixed_point

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

clean:
	rm -rf $(RESULTS)
