#ifndef MISC_HEADER_INCLUDE_CYCLE_FIXTURE
#define MISC_HEADER_INCLUDE_CYCLE_FIXTURE
// The guard prevents duplicate definitions on recursion while this fixture deliberately cycles.
#include "misc-header-include-cycle.cpp"

int
headerIncludeCycleFixture()
{ return 0; }
#endif
