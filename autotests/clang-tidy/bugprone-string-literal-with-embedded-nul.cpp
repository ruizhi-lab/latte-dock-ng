#include <string>

std::string
truncatedLiteral()
{
    // The C-string constructor truncates the literal at its embedded NUL.
    return std::string("abc\0def");
}
