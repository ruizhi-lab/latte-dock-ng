// Negative fixture for scripts/test-clazy.py: must trigger the clazy qstring-arg check.
#include <QString>
QString probe(const QString &a, const QString &b)
{
    return QStringLiteral("%1 %2").arg(a).arg(b);
}
