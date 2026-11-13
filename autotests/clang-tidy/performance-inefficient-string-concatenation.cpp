#include <string>

std::string
concatenateRepeatedly(const std::string &first, const std::string &second)
{
    std::string result = first;
    for (int index = 0; index < 2; ++index) {
        // Keep operator+ in the loop to exercise the unnecessary temporary check.
        result = result + second;
    }
    return result;
}
