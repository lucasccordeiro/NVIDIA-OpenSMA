// ESBMC 8.2.0 (post-#4187) — C++20 `using enum X;` (UsingEnumDecl) is not
// supported by the converter.
//
// Reproduce:
//   esbmc --std c++20 using_enum_decl.cpp
//
// Output:
//   ERROR: unrecognized / unimplemented clang declaration UsingEnum
//     UsingEnumDecl ... col:27 Enum '...' 'E'
//   ERROR: CONVERSION ERROR

namespace ns {
enum class E { A, B };
}

using enum ns::E;  // <-- C++20: brings A and B into the enclosing scope

int main()
{
    return static_cast<int>(A);
}
