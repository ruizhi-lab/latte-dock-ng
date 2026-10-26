// Negative fixture for scripts/test-clazy.py: must trigger the clazy qenums check.
#include <QObject>
class Legacy : public QObject {
    Q_OBJECT
public:
    enum Mode { A, B };
    Q_ENUMS(Mode)
};
