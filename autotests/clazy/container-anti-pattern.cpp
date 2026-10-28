// Negative fixture for scripts/test-clazy.py: must trigger the clazy container-anti-pattern check.
#include <QHash>
#include <QString>
bool probe(const QHash<QString, int> &hash, const QString &key)
{
    return hash.keys().contains(key);
}
