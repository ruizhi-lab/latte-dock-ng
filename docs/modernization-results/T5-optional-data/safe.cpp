#include <optional>

int readChecked(const std::optional<int> &value)
{
    if (!value.has_value()) {
        return 0;
    }
    return *value;
}
