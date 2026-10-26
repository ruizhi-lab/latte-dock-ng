// Negative fixture for scripts/test-clazy.py: must trigger the clazy connect-non-signal check.
#include <QObject>
class Emitter : public QObject {
    Q_OBJECT
public:
    void notASignal();
};
void probe(Emitter *emitter, QObject *context)
{
    QObject::connect(emitter, &Emitter::notASignal, context, [] {});
}
