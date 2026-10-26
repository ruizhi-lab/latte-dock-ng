// Negative fixture for scripts/test-clazy.py: must trigger the clazy install-event-filter check.
#include <QObject>
class Filter : public QObject {
    Q_OBJECT
public:
    explicit Filter(QObject *filterObject) { installEventFilter(filterObject); }
};
