// Auto-generated harness for: libspdm_copy_mem
// Source: corepdk/modules/spdm/src/app/libspdm/os_stub/memlib/copy_mem.c:10
//
// Bug claim: the three precondition checks at copy_mem.c:19,22,25 call
// LIBSPDM_ASSERT(0) but never return. With debuglib_null/debuglib.c selected
// (production embedded config), libspdm_debug_assert is an empty function,
// so a NULL dst_buf is silently accepted and the copy loop dereferences it.
//
// This harness invokes the function with dst_buf = NULL, src_buf pointing
// at a valid byte, and src_len = 1. ESBMC's default --pointer-check should
// flag the NULL dereference at line 30 (`*(dst++) = *(src++)`).

#include <stdint.h>
#include <stddef.h>

extern void libspdm_copy_mem(void *dst_buf, size_t dst_len,
                             const void *src_buf, size_t src_len);

void __byterepair_harness_main(void) {
    uint8_t src_storage = 0xAB;
    void *dst_buf = (void *)0;          // NULL — bug trigger
    const void *src_buf = &src_storage; // valid
    size_t dst_len = 16;                // arbitrary, > src_len
    size_t src_len = 1;                 // one byte to copy → one NULL write

    libspdm_copy_mem(dst_buf, dst_len, src_buf, src_len);

    // If we reach here, the function returned without crashing despite NULL dst.
    // ESBMC's --pointer-check should have already flagged the NULL deref before
    // this point; an unviolated harness would mean the bug is masked.
}
