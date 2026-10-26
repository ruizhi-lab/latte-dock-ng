// Negative fixture for scripts/test-clazy.py: must trigger the clazy auto-unexpected-qstringbuilder check.
#include <QString>
QString name();
void probe()
{
    const auto path = name() + QLatin1Char('/') + name();
    Q_UNUSED(path)
}
