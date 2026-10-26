// Negative fixture for scripts/test-clazy.py: must trigger the clazy post-event check.
#include <QCoreApplication>
#include <QEvent>
void probe(QObject *receiver)
{
    QEvent event(QEvent::User);
    QCoreApplication::postEvent(receiver, &event);
}
