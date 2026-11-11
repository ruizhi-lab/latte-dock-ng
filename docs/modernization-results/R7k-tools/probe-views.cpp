// Probe: the same KConfig calls as Layouts::Storage::views(const QString &file) and isLatteContainment().
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QStringList>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        qWarning() << "usage: probe-views FILE";
        return 2;
    }
    const QString file = QString::fromLocal8Bit(argv[1]);
    KSharedConfigPtr lFile = KSharedConfig::openConfig(file);
    KConfigGroup containmentGroups = KConfigGroup(lFile, QStringLiteral("Containments"));
    const QStringList ids = containmentGroups.groupList();
    qInfo().noquote() << "file:" << file << "exists:" << QFileInfo::exists(file);
    qInfo().noquote() << "groupList count:" << ids.count() << "ids:" << ids;
    for (const QString &cId : ids) {
        KConfigGroup g = containmentGroups.group(cId);
        qInfo().noquote() << "  cId" << cId << "name()" << g.name() << "plugin" << g.readEntry(QStringLiteral("plugin"), QString());
    }
    return 0;
}
