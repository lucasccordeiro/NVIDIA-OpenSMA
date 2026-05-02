// Concrete counterexample for F-6 (derived from ESBMC CEX).
// request_mode = 0xFF (255) — not Disable (0x00) or Enable (0x01).
// resp.mode is stored as 255 → assert fires.

extern "C" {

void __VERIFIER_assume(int cond) { (void)cond; }

unsigned char __VERIFIER_nondet_uchar(void) {
    static int i = 0;
    static const unsigned char v[] = { 0xFF };
    return v[i++];
}

} // extern "C"
