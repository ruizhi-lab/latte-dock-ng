// Negative fixture for scripts/test-clazy.py: must trigger the clazy qdeleteall check.
#include <QHash>
#include <QObject>
void probe(QHash<int, QObject *> &hash)
{
    qDeleteAll(hash.values());
}
