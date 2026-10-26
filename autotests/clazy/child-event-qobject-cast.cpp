// Negative fixture for scripts/test-clazy.py: must trigger the clazy child-event-qobject-cast check.
#include <QChildEvent>
#include <QObject>
class Target : public QObject { Q_OBJECT };
class Parent : public QObject {
    Q_OBJECT
protected:
    void childEvent(QChildEvent *event) override
    {
        Target *target = qobject_cast<Target *>(event->child());
        Q_UNUSED(target)
    }
};
