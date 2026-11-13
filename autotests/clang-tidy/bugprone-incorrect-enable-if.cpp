#include <type_traits>

// Keep the missing nested ::type so the condition does not constrain this overload.
template<typename T, typename = std::enable_if<T::value>>
void
enabledForValue(T)
{
}
