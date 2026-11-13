#include <stdexcept>

void
constructExceptionWithoutThrow()
{
    // Construct without throwing so this fixture remains valid under -fno-exceptions.
    std::runtime_error("failure");
}
