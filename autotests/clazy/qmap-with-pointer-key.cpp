// Negative fixture for scripts/test-clazy.py: must trigger the clazy qmap-with-pointer-key check.
#include <QMap>
QMap<int *, int> pointerKeys;
