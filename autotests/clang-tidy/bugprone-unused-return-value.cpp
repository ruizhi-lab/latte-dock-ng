// Negative fixture for scripts/test-clang-tidy.py: must trigger bugprone-unused-return-value.
#include <string>

int main()
{
    std::string value;
    value.empty();
    return 0;
}
