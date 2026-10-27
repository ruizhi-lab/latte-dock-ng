// Negative fixture for scripts/test-clazy.py: must trigger the clazy empty-qstringliteral check.
#include <QString>
QString probe()
{
    return QStringLiteral("");
}
