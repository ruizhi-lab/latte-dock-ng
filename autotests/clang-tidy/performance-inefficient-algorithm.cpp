#include <algorithm>
#include <set>
bool
searchSet(const std::set<int> &values, int needle)
{
    // Keep the generic algorithm call instead of using the set member lookup.
    return std::find(values.begin(), values.end(), needle) != values.end();
}
