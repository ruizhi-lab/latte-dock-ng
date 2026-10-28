// Negative fixture for scripts/test-clazy.py: must trigger the clazy fully-qualified-moc-types check.
#include <QByteArray>
#include <QObject>
namespace Outer {
namespace Data {
struct Layout {};
}
using Id = QByteArray;
class Emitter : public QObject {
    Q_OBJECT
Q_SIGNALS:
    void changed(Id id);
    void layoutChanged(const Data::Layout &layout);
public Q_SLOTS:
    void onChanged(Id id);
};
}
