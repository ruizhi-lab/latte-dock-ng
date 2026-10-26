// Negative fixture for scripts/test-clazy.py: must trigger the clazy mutable-container-key check.
#include <QMap>
#include <QObject>
#include <QPointer>
QMap<QPointer<QObject>, int> weakKeys;
