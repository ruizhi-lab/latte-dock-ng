#include <algorithm>
#include <vector>

void
eraseMatches(std::vector<int> &values)
{
    // The one-argument erase leaves the tail returned by remove in the container.
    values.erase(std::remove(values.begin(), values.end(), 1));
}
