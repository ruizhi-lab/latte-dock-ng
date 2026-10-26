// Negative fixture for scripts/test-clazy.py: must trigger the clazy temporary-iterator check.
#include <QList>
#include <QString>
QList<QString> makeList();
int probe()
{
    int count = 0;
    for (auto it = makeList().begin(); it != makeList().end(); ++it) {
        ++count;
    }
    return count;
}
