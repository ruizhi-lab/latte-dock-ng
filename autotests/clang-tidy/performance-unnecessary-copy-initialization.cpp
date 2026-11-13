#include <string>

std::string::size_type
copiedStringLength(const std::string &source)
{
    // Keep this negative fixture's expensive string copy unchanged so the check must reject it.
    const std::string copy = source;
    return copy.size();
}
