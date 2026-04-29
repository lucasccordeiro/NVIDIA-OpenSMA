#include <cstdint>
struct S { uint8_t a; uint16_t b; };  // internal padding between a and b
class C { public: C() = default; S s{}; };
int main() { C c{}; (void)c; return 0; }
