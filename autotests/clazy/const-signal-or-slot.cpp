// Negative fixture for scripts/test-clazy.py: must trigger the clazy const-signal-or-slot check.
#include <QObject>
class Getter : public QObject {
    Q_OBJECT
public Q_SLOTS:
    int value() const;
};
