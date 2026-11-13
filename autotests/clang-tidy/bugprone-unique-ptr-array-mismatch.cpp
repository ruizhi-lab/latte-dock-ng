#include <memory>

void
uniquePointerArrayMismatch()
{ std::unique_ptr<int> values(new int[3]); }
