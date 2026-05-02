// Concrete counterexample for F-8 (derived from ESBMC CEX).
// ei_gpio_number = 17 (one beyond MaxGPIOSpoofingEntries = 16).
// The validator loop reaches i = 16, reading gpio_ei_entries[16] which is
// one past the end of the array — ASan reports a stack-buffer-overflow.

extern "C" {

void __VERIFIER_assume(int cond) { (void)cond; }

unsigned short __VERIFIER_nondet_ushort(void) { return 17; }
unsigned char  __VERIFIER_nondet_uchar(void)  { return 0; }

} // extern "C"
