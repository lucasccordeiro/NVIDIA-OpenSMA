// ESBMC 8.2.0 libc++ shim: <span> is not bundled.
// Reproduce: esbmc --std c++20 stdlib_gap_span.cpp
#include <span>
int main() { return 0; }
