// Negative fixture for scripts/test-clazy.py: must trigger the clazy strict-iterators check.
#include <QHash>
bool probe(QHash<int, int> &hash)
{
    return hash.find(1) == hash.cend();
}
