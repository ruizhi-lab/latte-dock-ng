#include <string>

bool
hasCharacter(const std::string &value)
{
    // Keep the one-character literal to exercise the char overload suggestion.
    return value.find("x") != std::string::npos;
}
