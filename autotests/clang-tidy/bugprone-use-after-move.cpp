// Negative fixture for scripts/test-clang-tidy.py: must trigger bugprone-use-after-move.
#include <string>
#include <utility>

int main()
{
    std::string moved = "latte";
    std::string target = std::move(moved);
    return static_cast<int>(moved.size() + target.size());
}
