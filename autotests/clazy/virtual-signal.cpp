// Negative fixture for scripts/test-clazy.py: must trigger the clazy virtual-signal check.
#include <QObject>
class Emitter : public QObject {
    Q_OBJECT
Q_SIGNALS:
    virtual void changed();
};
