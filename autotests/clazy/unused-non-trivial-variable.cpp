// Negative fixture for scripts/test-clazy.py: must trigger the clazy unused-non-trivial-variable check.
#include <QString>
void probe(int value)
{
    QString text = QString::number(value);
}
