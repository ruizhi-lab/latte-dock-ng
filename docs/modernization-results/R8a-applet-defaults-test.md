# Applet model default selection

The production Applets model is now tested with each of the seven plugin IDs
in its current no-personal-data list and an ordinary task applet. The test
checks both its selection role and Qt check state. It covers model defaults,
not the contents of an exported layout or a broader privacy guarantee. The
existing source contract remains in place.

Seven isolated source mutations removed one listed ID at a time. Each caused
its corresponding test row to fail while the other rows passed; production
source was not modified. The affected `modelunittest` target built without
compiler warnings on GCC and Clang Debug and Release, and its focused CTest
passed in each configuration. No production behavior changed. Remote CI is
pending.
