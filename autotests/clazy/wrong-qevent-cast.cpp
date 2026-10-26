// Negative fixture for scripts/test-clazy.py: must trigger the clazy wrong-qevent-cast check.
#include <QEvent>
#include <QKeyEvent>
int probe(QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseMove: {
        auto *key = static_cast<QKeyEvent *>(event);
        return key->key();
    }
    default:
        return 0;
    }
}
