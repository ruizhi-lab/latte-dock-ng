// Negative fixture for scripts/test-clazy.py: must trigger the clazy overridden-signal check.
#include <QObject>
class Base : public QObject {
    Q_OBJECT
Q_SIGNALS:
    void changed();
};
class Derived : public Base {
    Q_OBJECT
Q_SIGNALS:
    void changed();
};
