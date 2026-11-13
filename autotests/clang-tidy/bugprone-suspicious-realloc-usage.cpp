#include <cstdlib>
void
reallocIntoSamePointer()
{
    void *buffer = std::malloc(8);
    // Assign back to the input pointer to exercise the realloc-loss check.
    buffer = std::realloc(buffer, 16);
    std::free(buffer);
}
