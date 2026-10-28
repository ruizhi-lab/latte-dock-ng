// Negative fixture for scripts/test-clazy.py: must trigger the clazy incorrect-emit check.
#include <QObject>
class Emitter : public QObject {
    Q_OBJECT
public:
    Emitter() { Q_EMIT changed(); }
    void poke() { changed(); }
Q_SIGNALS:
    void changed();
};
