#include <memory>

void
transferOwnership(std::unique_ptr<int> &destination, std::unique_ptr<int> &source)
{
    // Preserve reset(release()) so the check recommends a move assignment.
    destination.reset(source.release());
}
