#include <climits>
#include <cstddef>
#include <cstring>

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }

    if (std::strcmp(argv[1], "address") == 0) {
        auto *values = new int[1]{0};
        volatile std::size_t index = 2;
        return values[index];
    }

    if (std::strcmp(argv[1], "undefined") == 0) {
        volatile int value = INT_MAX;
        return value + 1;
    }

    return 2;
}
