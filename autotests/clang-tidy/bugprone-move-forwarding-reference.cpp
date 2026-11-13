#include <utility>
void
consume(int);
template<typename T>
void
forwardAsRvalue(T &&value)
{
    // Keep std::move on the forwarding reference so lvalues are incorrectly treated as rvalues.
    consume(std::move(value));
}
