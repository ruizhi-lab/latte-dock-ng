// Negative fixture for scripts/test-clazy.py: must trigger the clazy qgetenv check.
#include <QByteArray>
bool probe()
{
    return qgetenv("LATTE_PROBE").isEmpty();
}
