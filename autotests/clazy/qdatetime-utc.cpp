// Negative fixture for scripts/test-clazy.py: must trigger the clazy qdatetime-utc check.
#include <QDateTime>
QDateTime probe()
{
    return QDateTime::currentDateTime().toUTC();
}
