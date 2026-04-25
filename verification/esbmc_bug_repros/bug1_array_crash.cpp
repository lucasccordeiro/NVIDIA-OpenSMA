// ESBMC 8.2.0 — bug 1 reproducer
// std::array instantiation crashes the C++ converter.
// Crash:
//   Assertion failed: (ctor_class_symb), function gen_vptr_initializations,
//                     file clang_cpp_adjust_code_gen.cpp, line 48.
// Reproduce:
//   esbmc --std c++20 bug1_array_crash.cpp
#include <array>
#include <cstdint>

int main()
{
    std::array<uint8_t, 4> a{};
    return a[0];
}
