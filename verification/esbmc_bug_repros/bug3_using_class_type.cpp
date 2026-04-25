// ESBMC 8.2.0 — bug 3 reproducer
// using-declaration that brings a class type into the current scope, when
// that name is then used as a type-name, fails conversion with:
//   ERROR: Conversion of unsupported clang type: "Using
//   UsingType '<name>' sugar '<fully-qualified>'
// A using-alias (`using Packet = ns::Packet;`) avoids the crash; the
// using-declaration form (`using ns::Packet;`) does not.
//
// Reproduce:
//   esbmc --std c++20 bug3_using_class_type.cpp
namespace ns {
struct Packet
{
    int x;
};
}  // namespace ns

using ns::Packet;  // <-- using-declaration of a class type

int main()
{
    Packet pkt{};  // <-- name used as a type-name => conversion error
    return pkt.x;
}
