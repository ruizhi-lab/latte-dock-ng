#include <memory>

void
sharedPointerArrayMismatch()
{ std::shared_ptr<int> values(new int[3]); }
