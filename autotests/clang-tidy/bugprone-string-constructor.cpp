#include <string_view>

void
emptyView()
{
    // Keep a non-empty literal paired with a zero length to trigger the constructor check.
    std::string_view empty("test", 0);
    (void)empty;
}
