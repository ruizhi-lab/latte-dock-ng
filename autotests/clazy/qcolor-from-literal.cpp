// Negative fixture for scripts/test-clazy.py: must trigger the clazy qcolor-from-literal check.
#include <QColor>
QColor probe()
{
    return QColor("#112233");
}
