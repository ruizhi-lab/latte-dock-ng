// Negative fixture for scripts/test-clazy.py: must trigger the clazy lowercase-qml-type-name check.
#include <QObject>
#include <QtQml/qqml.h>
class Item : public QObject { Q_OBJECT };
void probe()
{
    qmlRegisterType<Item>("org.example", 1, 0, "item");
}
