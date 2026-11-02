// Negative fixture for scripts/test-clazy.py: must trigger the clazy qproperty-without-notify check.
#include <QObject>
class Holder : public QObject {
    Q_OBJECT
    Q_PROPERTY(int value READ value)
public:
    int value() const;
};
