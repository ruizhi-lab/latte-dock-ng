// Negative fixture for scripts/test-clazy.py: must trigger the clazy detaching-temporary check.
#include <QList>
#include <QString>
QList<QString> makeList();
QString probe()
{
    return makeList().first();
}
