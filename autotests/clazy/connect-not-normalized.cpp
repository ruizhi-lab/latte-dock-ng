// Negative fixture for scripts/test-clazy.py: must trigger the clazy connect-not-normalized check.
#include <QMetaObject>
#include <QObject>
void probe(QObject *target, QObject *argument)
{
    QMetaObject::invokeMethod(target, "take", Q_ARG(QObject *, argument));
}
