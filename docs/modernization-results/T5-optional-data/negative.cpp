#include <optional>

int readUnchecked(const std::optional<int> &value)
{
    return *value;
}
