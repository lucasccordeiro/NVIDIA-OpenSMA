// ESBMC 8.2.0 libc++ shim: <type_traits> is incomplete. Variable
// templates is_enum_v / is_integral_v / is_unsigned_v are missing,
// and so are enable_if_t / is_same_v in some configurations.
// Reproduce: esbmc --std c++20 stdlib_gap_type_traits.cpp
#include <type_traits>
template <typename T>
requires(std::is_enum_v<T>)
constexpr int kind() { return 1; }

enum class E { A };
int main() { return kind<E>(); }
