// Negative fixture for scripts/test-clazy.py: must trigger the clazy qlatin1string-non-ascii check.
#include <QLatin1String>
QLatin1String probe()
{
    return QLatin1String("é");
}
