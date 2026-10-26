// Negative fixture for scripts/test-clazy.py: must trigger the clazy connect-by-name check.
#include <QObject>
class Receiver : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void on_button_clicked();
};
