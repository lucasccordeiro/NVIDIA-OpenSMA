// ESBMC verification stub for mbedtls/ctr_drbg.h
// Provides the minimum type and function declarations used by nsm.cpp.
// Only mbedtls_ctr_drbg_context and three functions are needed; the
// context is opaque to every call site outside mbedtls itself.
#pragma once
#include <cstddef>
#include <cstdint>

typedef struct { int dummy; } mbedtls_ctr_drbg_context;

static inline void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context*) {}
static inline int  mbedtls_ctr_drbg_random(void*, unsigned char*, size_t) { return 0; }
static inline void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context*) {}
