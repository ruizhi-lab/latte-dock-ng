// Negative fixture for scripts/test-clazy.py: must trigger the clazy range-loop-reference check.
#include <QStringList>
int probe(const QStringList &names)
{
    int total = 0;
    for (auto name : names) {
        total += name.size();
    }
    return total;
}
