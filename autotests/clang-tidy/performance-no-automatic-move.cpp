#include <optional>
#include <string>

std::optional<std::string>
constLocalConversionReturn(bool useFirst)
{
    if (useFirst) {
        const std::string first{ "first" };
        return first;
    }

    const std::string second{ "second" };
    return second;
}
