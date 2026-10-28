// Negative fixture for scripts/test-clazy.py: must trigger the clazy overloaded-signal check.
#include <QObject>
class Base : public QObject {
    Q_OBJECT
Q_SIGNALS:
    void changed(int value);
};
class Derived : public Base {
    Q_OBJECT
Q_SIGNALS:
    void changed();
};
