// Concrete counterexample for F-13 (simplified from ESBMC CEX).
// x = -1024 (SFXP22_10): represents exactly -1% (1024 raw units = 1.0 in 10-bit fraction).
// sfxp22_10_to_sfxp32_0(-1024) = -1024 >> 10 = -1.
// static_cast<uint8_t>(-1) = 255 > 150 → assert fires.

extern "C" {

void __VERIFIER_assume(int cond) { (void)cond; }

int __VERIFIER_nondet_int(void) {
    static int i = 0;
    static const int v[] = { -1024 };
    return v[i++];
}

} // extern "C"
