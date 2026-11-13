#include <optional>

void
consume(std::optional<int> value);

void
forwardOptionalValue(std::optional<int> value)
{
    // Extracting and rewrapping the value discards the optional's empty state.
    consume(value.value());
}
