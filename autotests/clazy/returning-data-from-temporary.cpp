// Negative fixture for scripts/test-clazy.py: must trigger the clazy returning-data-from-temporary check.
#include <QByteArray>
#include <QString>
const char *probe(const QString &text)
{
    return text.toUtf8().constData();
}
