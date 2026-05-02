// Concrete counterexample for F-10 (derived from ESBMC CEX).
// threshold = 254 → static_cast<int16_t>(254) = 254 > NtcTempMax (125).
// ntc_temperature_to_resistance(254) returns 0 → silent 125°C substitution.
// Function returns Ccode::Success → assert fires.

extern "C" {

void __VERIFIER_assume(int cond) { (void)cond; }

unsigned char __VERIFIER_nondet_uchar(void) {
    static int i = 0;
    static const unsigned char v[] = { 254 };
    return v[i++];
}

} // extern "C"
