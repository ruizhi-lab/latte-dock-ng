#include <map>
#include <string>
#include <vector>

void
implicitConversion(const std::map<int, std::vector<std::string>> &values)
{
    for (const std::pair<int, std::vector<std::string>> &entry : values) {
        (void)entry;
    }
}
