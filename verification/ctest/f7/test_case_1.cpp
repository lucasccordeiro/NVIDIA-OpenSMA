// Concrete counterexample for F-7 (derived from ESBMC CEX).
// incoming.offset = 42 (nonzero), all other fields = 0.
// Validation fails (__VERIFIER_nondet_uint returns 0 → 0 % 2 = 0 → valid = false).
// After the failed validation, stored.offset = 42 → is_zero_initialised returns
// false → assert fires.

extern "C" {

void __VERIFIER_assume(int cond) { (void)cond; }

static int uchar_i = 0;
// Fields in order: offset, error_injection_id, error_type,
//                  l1_bitmap, l2_bitmap, l3_bitmap, reserved_1
static const unsigned char uchar_vals[] = { 42, 0, 0, 0, 0, 0, 0 };
unsigned char __VERIFIER_nondet_uchar(void) { return uchar_vals[uchar_i++]; }

unsigned int __VERIFIER_nondet_uint(void) { return 0; }  // 0 % 2 = 0 → valid = false

} // extern "C"
