#include <cstring>
struct NonTrivial
{
    NonTrivial() = default;
    NonTrivial(const NonTrivial &) {}
    int value = 0;
};
void
copyRepresentation(NonTrivial *to, const NonTrivial *from)
{
    // Copy object bytes into a non-trivially-copyable destination.
    std::memcpy(to, from, sizeof(*to));
}
