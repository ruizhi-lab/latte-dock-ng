// Negative fixture for scripts/test-clazy.py: must trigger the clazy qstring-insensitive-allocation check.
#include <QString>
bool probe(const QString &text)
{
    return text.toLower().contains(QStringLiteral("foo"));
}
