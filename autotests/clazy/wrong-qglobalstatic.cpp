// Negative fixture for scripts/test-clazy.py: must trigger the clazy wrong-qglobalstatic check.
#include <QGlobalStatic>
struct Trivial { int value; };
Q_GLOBAL_STATIC(Trivial, trivialInstance)
