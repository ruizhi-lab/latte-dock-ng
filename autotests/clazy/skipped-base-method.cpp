// Negative fixture for scripts/test-clazy.py: must trigger the clazy skipped-base-method check.
#include <QObject>
class Base : public QObject {
    Q_OBJECT
public:
    virtual void doIt() {}
};
class Middle : public Base {
    Q_OBJECT
public:
    void doIt() override {}
};
class Leaf : public Middle {
    Q_OBJECT
public:
    void doIt() override { Base::doIt(); }
};
