// Negative fixture for scripts/test-clazy.py: must trigger the clazy qfileinfo-exists check.
#include <QFileInfo>
bool probe(const QString &path)
{
    return QFileInfo(path).exists();
}
