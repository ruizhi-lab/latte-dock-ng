#include <optional>

int
readUnchecked(const std::optional<int> &value)
{
    // Dereferencing without proving the optional holds a value is the defect the check reports.
    return *value;
}
