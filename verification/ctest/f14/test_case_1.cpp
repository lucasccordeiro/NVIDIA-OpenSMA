// Concrete counterexample for F-14 (derived from ESBMC CEX).
// cmd_byte = 0x00 → Pca9555Register::Input.
// data = {0x00, 0xAB, 0xCD}, data_length = 3.
// simulate_i2c_write_iterations returns 0 (bare return at first iteration).
// expected = 2 → assert fires.

extern "C" {

void __VERIFIER_assume(int cond) { (void)cond; }

static int uchar_i = 0;
static const unsigned char uchar_vals[] = { 0xAB, 0xCD };
unsigned char __VERIFIER_nondet_uchar(void) { return uchar_vals[uchar_i++]; }

} // extern "C"
