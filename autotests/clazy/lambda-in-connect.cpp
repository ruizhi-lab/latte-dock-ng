// Negative fixture for scripts/test-clazy.py: must trigger the clazy lambda-in-connect check.
#include <QObject>
class Emitter : public QObject {
    Q_OBJECT
Q_SIGNALS:
    void changed();
};
void probe(Emitter *emitter, QObject *context)
{
    int local = 0;
    QObject::connect(emitter, &Emitter::changed, context, [&local] { ++local; });
}
