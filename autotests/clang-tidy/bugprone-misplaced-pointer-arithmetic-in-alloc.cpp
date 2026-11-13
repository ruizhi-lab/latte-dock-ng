#include <cstdlib>
char *
misplacedAllocationOffset(int size)
{
    // Keep the offset on the allocation result instead of its size argument.
    return static_cast<char *>(std::malloc(size)) + 1;
}
