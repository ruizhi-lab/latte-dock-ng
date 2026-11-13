#include <iostream>
void
flushWithEndl()
{
    // Keep std::endl to exercise the unnecessary flush check.
    std::cout << "value" << std::endl;
}
