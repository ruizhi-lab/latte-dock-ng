// Negative fixture for scripts/test-clazy.py: must trigger the clazy lambda-unique-connection check.
#include <QObject>
class Emitter : public QObject {
    Q_OBJECT
Q_SIGNALS:
    void changed();
};
void probe(Emitter *emitter, QObject *context)
{
    QObject::connect(emitter, &Emitter::changed, context, [] {}, Qt::UniqueConnection);
}
