// Negative fixture for scripts/test-clazy.py: must trigger the clazy connect-3arg-lambda check.
#include <QObject>
#include <QTimer>
void probe(QObject *target)
{
    QTimer::singleShot(100, [target]() { target->deleteLater(); });
}
