// ESBMC 8.2.0 — bug 2 reproducer
// `constexpr` at namespace scope, initialised from a value qualified by an
// inner namespace, crashes the C++ converter.
// Crash:
//   Assertion failed: (Kind == StoredKind::Type), function getAsType,
//                     file NestedNameSpecifierBase.h, line 159.
// Reproduce:
//   esbmc --std c++20 bug2_qualified_constexpr.cpp
#include <cstdint>

namespace outer {
namespace inner {
constexpr unsigned long Value = 64;
}  // namespace inner
}  // namespace outer

namespace outer {
struct Header
{
    uint8_t a;
};
// Trigger: qualified `inner::Value` used in a sibling-namespace constexpr
// initializer involving sizeof. Removing the `inner::` qualifier (e.g. via
// `using inner::Value;` first) avoids the crash.
constexpr unsigned long DataLen = inner::Value + sizeof(Header);
}  // namespace outer

int main()
{
    return (int)outer::DataLen;
}
