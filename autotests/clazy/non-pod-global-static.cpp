// Negative fixture for scripts/test-clazy.py: must trigger the clazy non-pod-global-static check.
#include <QString>
namespace {
const QString fixtureName = QStringLiteral("fixture");
}
int probe()
{
    return fixtureName.size();
}
