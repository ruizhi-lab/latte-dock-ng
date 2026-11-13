#include <cstring>
struct NonStandard
{
    virtual ~NonStandard() = default;
    int value;
};
bool
compareObjects(const NonStandard &a, const NonStandard &b)
{
    // Compare object representations to exercise the non-standard-layout diagnostic.
    return std::memcmp(&a, &b, sizeof(a)) == 0;
}
