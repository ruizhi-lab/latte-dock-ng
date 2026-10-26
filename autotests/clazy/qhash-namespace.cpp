// Negative fixture for scripts/test-clazy.py: must trigger the clazy qhash-namespace check.
#include <QHashFunctions>
namespace Probe { struct Key { int value; }; }
size_t qHash(const Probe::Key &key, size_t seed = 0) { return qHash(key.value, seed); }
