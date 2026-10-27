// Negative fixture for scripts/test-clazy.py: must trigger the clazy use-static-qregularexpression check.
#include <QRegularExpression>
#include <QString>
bool probe(const QString &text)
{
    return text.contains(QRegularExpression(QStringLiteral("^a+$")));
}
