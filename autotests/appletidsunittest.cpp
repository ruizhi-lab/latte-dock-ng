/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../app/layouts/appletids.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QFile>
#include <QHash>
#include <QSet>
#include <QTemporaryDir>
#include <QTest>

using namespace Latte::Layouts;

namespace {

const QString APPLETS = QStringLiteral("Applets");
const QString GENERAL = QStringLiteral("General");
const QString PLUGIN = QStringLiteral("plugin");

KConfigGroup addApplet(KConfigGroup host, const QString &id, const QString &plugin)
{
    KConfigGroup applet = host.group(APPLETS).group(id);
    applet.writeEntry(PLUGIN, plugin);
    return applet;
}

//! plugin -> id of every applet below host, nested ones included; the
//! fixtures use each plugin once so the plugin identifies the applet.
void idsByPlugin(const KConfigGroup &host, QHash<QString, QString> &result)
{
    const KConfigGroup applets = host.group(APPLETS);

    for (const auto &id : applets.groupList()) {
        result.insert(applets.group(id).readEntry(PLUGIN, QString()), id);
        idsByPlugin(applets.group(id), result);
    }
}

QHash<QString, QString> idsByPlugin(const QList<KConfigGroup> &containments)
{
    QHash<QString, QString> result;

    for (const auto &containment : containments) {
        idsByPlugin(containment, result);
    }

    return result;
}

void allIds(const KConfigGroup &host, QStringList &result)
{
    const KConfigGroup applets = host.group(APPLETS);

    for (const auto &id : applets.groupList()) {
        result << id;
        allIds(applets.group(id), result);
    }
}

QStringList allIds(const QList<KConfigGroup> &containments)
{
    QStringList result;

    for (const auto &containment : containments) {
        allIds(containment, result);
    }

    return result;
}

QSet<uint> toIdSet(std::initializer_list<uint> ids, uint rangeFirst = 0, uint rangeLast = 0)
{
    QSet<uint> result(ids);

    for (uint id = rangeFirst; rangeLast > 0 && id <= rangeLast; ++id) {
        result.insert(id);
    }

    return result;
}

QByteArray fileContents(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

void verifyUniqueAndFree(const QList<KConfigGroup> &containments, const QSet<uint> &usedIds)
{
    const QStringList ids = allIds(containments);
    QSet<QString> seen;

    for (const auto &id : ids) {
        QVERIFY2(!seen.contains(id), qPrintable(QStringLiteral("duplicated applet id %1").arg(id)));
        QVERIFY2(!usedIds.contains(id.toUInt()), qPrintable(QStringLiteral("applet id %1 is already used").arg(id)));
        seen.insert(id);
    }
}

}

class AppletIdsUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();

    void collectIncludesNestedApplets();
    void collectOfMissingGroupIsEmpty();
    void duplicatedTrayGetsUniqueNestedIds();
    void appletIdTakenByNewContainmentIsRenamed();
    void uniqueAppletsAreLeftUntouched();
    void repeatedIdsInsideTheImportAreSeparated();
    void renamedAppletNeverMergesIntoAnUnvisitedSibling();
    void deeplyNestedAppletsAreResolved();
    void nonNumericGroupsAreIgnored();

private:
    QTemporaryDir m_dir;
    KSharedConfigPtr m_config;
    KConfigGroup containment(const QString &id);
};

void AppletIdsUnitTest::init()
{
    QVERIFY(m_dir.isValid());
    m_config = KSharedConfig::openConfig(m_dir.filePath(QStringLiteral("%1.layout.latte").arg(QString::fromLatin1(QTest::currentTestFunction()))),
                                         KConfig::SimpleConfig);
}

void AppletIdsUnitTest::cleanup()
{
    m_config.reset();
}

KConfigGroup AppletIdsUnitTest::containment(const QString &id)
{
    return KConfigGroup(m_config, QStringLiteral("Containments")).group(id);
}

void AppletIdsUnitTest::collectIncludesNestedApplets()
{
    KConfigGroup dock = containment(QStringLiteral("3"));
    addApplet(dock, QStringLiteral("40"), QStringLiteral("org.kde.latte.plasmoid"));
    KConfigGroup tray = addApplet(dock, QStringLiteral("41"), QStringLiteral("org.kde.plasma.systemtray"));
    addApplet(tray, QStringLiteral("42"), QStringLiteral("org.kde.kdeconnect"));
    KConfigGroup host = addApplet(tray, QStringLiteral("43"), QStringLiteral("org.example.host"));
    addApplet(host, QStringLiteral("44"), QStringLiteral("org.example.inner"));

    QStringList ids = AppletIds::collect(dock.group(APPLETS));
    ids.sort();
    QCOMPARE(ids, QStringList({QStringLiteral("40"), QStringLiteral("41"), QStringLiteral("42"), QStringLiteral("43"), QStringLiteral("44")}));
}

void AppletIdsUnitTest::collectOfMissingGroupIsEmpty()
{
    QVERIFY(AppletIds::collect(containment(QStringLiteral("3")).group(APPLETS)).isEmpty());
}

void AppletIdsUnitTest::duplicatedTrayGetsUniqueNestedIds()
{
    // The copy of a dock whose system tray (libplasma 6.8 layout) holds its
    // items as nested applets with the ids of the original tray items.
    KConfigGroup copy = containment(QStringLiteral("45"));
    copy.group(GENERAL).writeEntry(QStringLiteral("appletOrder"), QStringLiteral("61;60"));
    addApplet(copy, QStringLiteral("60"), QStringLiteral("org.kde.latte.plasmoid"));
    KConfigGroup tray = addApplet(copy, QStringLiteral("61"), QStringLiteral("org.kde.plasma.systemtray"));
    tray.group(GENERAL).writeEntry(QStringLiteral("extraItems"), QStringLiteral("org.kde.kdeconnect,org.kde.plasma.volume"));
    addApplet(tray, QStringLiteral("42"), QStringLiteral("org.kde.kdeconnect"));
    KConfigGroup volume = addApplet(tray, QStringLiteral("43"), QStringLiteral("org.kde.plasma.volume"));
    volume.group(QStringLiteral("Configuration")).group(GENERAL).writeEntry(QStringLiteral("migrated"), true);

    // The original dock: containment 3, applets 40 and 41, tray items 42 and 43.
    const QSet<uint> usedIds = toIdSet({3, 40, 41, 42, 43, 45});
    const QList<KConfigGroup> imported{copy};
    AppletIds::makeUnique(imported, usedIds);

    verifyUniqueAndFree(imported, usedIds);
    if (QTest::currentTestFailed()) {
        return;
    }

    QCOMPARE(allIds(imported).count(), 4);

    const QHash<QString, QString> ids = idsByPlugin(imported);
    QCOMPARE(ids.value(QStringLiteral("org.kde.latte.plasmoid")), QStringLiteral("60"));
    QCOMPARE(ids.value(QStringLiteral("org.kde.plasma.systemtray")), QStringLiteral("61"));
    QCOMPARE(copy.group(GENERAL).readEntry(QStringLiteral("appletOrder"), QString()), QStringLiteral("61;60"));

    // Nested groups move with everything below them.
    const KConfigGroup movedTray = copy.group(APPLETS).group(QStringLiteral("61"));
    QCOMPARE(movedTray.group(GENERAL).readEntry(QStringLiteral("extraItems"), QString()), QStringLiteral("org.kde.kdeconnect,org.kde.plasma.volume"));
    const KConfigGroup movedVolume = movedTray.group(APPLETS).group(ids.value(QStringLiteral("org.kde.plasma.volume")));
    QCOMPARE(movedVolume.group(QStringLiteral("Configuration")).group(GENERAL).readEntry(QStringLiteral("migrated"), false), true);
    QVERIFY(!movedTray.group(APPLETS).hasGroup(QStringLiteral("42")));
    QVERIFY(!movedTray.group(APPLETS).hasGroup(QStringLiteral("43")));
}

void AppletIdsUnitTest::appletIdTakenByNewContainmentIsRenamed()
{
    // libplasma 6.8 gives the imported containment the next id after the
    // highest one in use, the same id newUniqueIdsFile() picks for the first
    // applet when the existing ids have no gaps.
    KConfigGroup dock = containment(QStringLiteral("60"));
    KConfigGroup general = dock.group(GENERAL);
    general.writeEntry(QStringLiteral("appletOrder"), QStringLiteral("61;60"));
    general.writeEntry(QStringLiteral("lockedZoomApplets"), QStringLiteral("60"));
    general.writeEntry(QStringLiteral("userBlocksColorizingApplets"), QStringLiteral("60;61"));
    addApplet(dock, QStringLiteral("60"), QStringLiteral("org.kde.latte.plasmoid"));
    addApplet(dock, QStringLiteral("61"), QStringLiteral("org.kde.plasma.systemtray"));

    const QSet<uint> usedIds = toIdSet({1, 2, 3}, 40, 60);
    const QList<KConfigGroup> imported{dock};
    AppletIds::makeUnique(imported, usedIds);

    verifyUniqueAndFree(imported, usedIds);
    if (QTest::currentTestFailed()) {
        return;
    }

    const QString tasks = idsByPlugin(imported).value(QStringLiteral("org.kde.latte.plasmoid"));
    QVERIFY(tasks != QStringLiteral("60"));
    QCOMPARE(idsByPlugin(imported).value(QStringLiteral("org.kde.plasma.systemtray")), QStringLiteral("61"));
    QCOMPARE(general.readEntry(QStringLiteral("appletOrder"), QString()), QStringLiteral("61;") + tasks);
    QCOMPARE(general.readEntry(QStringLiteral("lockedZoomApplets"), QString()), tasks);
    QCOMPARE(general.readEntry(QStringLiteral("userBlocksColorizingApplets"), QString()), tasks + QStringLiteral(";61"));
}

void AppletIdsUnitTest::uniqueAppletsAreLeftUntouched()
{
    KConfigGroup dock = containment(QStringLiteral("12"));
    dock.group(GENERAL).writeEntry(QStringLiteral("appletOrder"), QStringLiteral("50;51"));
    addApplet(dock, QStringLiteral("50"), QStringLiteral("org.kde.latte.plasmoid"));
    KConfigGroup tray = addApplet(dock, QStringLiteral("51"), QStringLiteral("org.kde.plasma.systemtray"));
    addApplet(tray, QStringLiteral("52"), QStringLiteral("org.kde.plasma.volume"));
    QVERIFY(m_config->sync());
    const QByteArray before = fileContents(m_config->name());
    QVERIFY(before.contains("[Containments][12][Applets][51][Applets][52]"));

    AppletIds::makeUnique({dock}, toIdSet({1, 12, 40}));

    QVERIFY(!m_config->isDirty());
    QVERIFY(m_config->sync());
    QCOMPARE(fileContents(m_config->name()), before);
}

void AppletIdsUnitTest::repeatedIdsInsideTheImportAreSeparated()
{
    // A copy made before nested ids were considered: the tasks applet and a
    // tray item share id 42, and a second imported dock repeats it again.
    KConfigGroup first = containment(QStringLiteral("59"));
    first.group(GENERAL).writeEntry(QStringLiteral("appletOrder"), QStringLiteral("43;42"));
    addApplet(first, QStringLiteral("42"), QStringLiteral("org.kde.latte.plasmoid"));
    KConfigGroup tray = addApplet(first, QStringLiteral("43"), QStringLiteral("org.kde.plasma.systemtray"));
    addApplet(tray, QStringLiteral("42"), QStringLiteral("org.kde.kdeconnect"));
    addApplet(tray, QStringLiteral("43"), QStringLiteral("org.kde.plasma.cameraindicator"));

    KConfigGroup second = containment(QStringLiteral("62"));
    second.group(GENERAL).writeEntry(QStringLiteral("appletOrder"), QStringLiteral("42"));
    addApplet(second, QStringLiteral("42"), QStringLiteral("org.kde.plasma.digitalclock"));

    const QSet<uint> usedIds = toIdSet({1, 59, 62});
    const QList<KConfigGroup> imported{first, second};
    AppletIds::makeUnique(imported, usedIds);

    verifyUniqueAndFree(imported, usedIds);
    if (QTest::currentTestFailed()) {
        return;
    }

    QCOMPARE(allIds(imported).count(), 5);

    const QHash<QString, QString> ids = idsByPlugin(imported);
    QCOMPARE(ids.value(QStringLiteral("org.kde.latte.plasmoid")), QStringLiteral("42"));
    QCOMPARE(ids.value(QStringLiteral("org.kde.plasma.systemtray")), QStringLiteral("43"));
    QCOMPARE(first.group(GENERAL).readEntry(QStringLiteral("appletOrder"), QString()), QStringLiteral("43;42"));
    QCOMPARE(second.group(GENERAL).readEntry(QStringLiteral("appletOrder"), QString()), ids.value(QStringLiteral("org.kde.plasma.digitalclock")));
}

void AppletIdsUnitTest::renamedAppletNeverMergesIntoAnUnvisitedSibling()
{
    KConfigGroup dock = containment(QStringLiteral("12"));
    addApplet(dock, QStringLiteral("40"), QStringLiteral("org.kde.latte.plasmoid"));
    addApplet(dock, QStringLiteral("41"), QStringLiteral("org.kde.plasma.systemtray"));

    const QSet<uint> usedIds = toIdSet({12, 40});
    const QList<KConfigGroup> imported{dock};
    AppletIds::makeUnique(imported, usedIds);

    verifyUniqueAndFree(imported, usedIds);
    if (QTest::currentTestFailed()) {
        return;
    }

    const QHash<QString, QString> ids = idsByPlugin(imported);
    QCOMPARE(ids.value(QStringLiteral("org.kde.plasma.systemtray")), QStringLiteral("41"));
    QCOMPARE(ids.value(QStringLiteral("org.kde.latte.plasmoid")), QStringLiteral("42"));
}

void AppletIdsUnitTest::deeplyNestedAppletsAreResolved()
{
    KConfigGroup dock = containment(QStringLiteral("12"));
    KConfigGroup outer = addApplet(dock, QStringLiteral("40"), QStringLiteral("org.example.outer"));
    KConfigGroup middle = addApplet(outer, QStringLiteral("41"), QStringLiteral("org.example.middle"));
    addApplet(middle, QStringLiteral("42"), QStringLiteral("org.example.inner"));

    const QSet<uint> usedIds = toIdSet({12}, 40, 42);
    const QList<KConfigGroup> imported{dock};
    AppletIds::makeUnique(imported, usedIds);

    verifyUniqueAndFree(imported, usedIds);
    if (QTest::currentTestFailed()) {
        return;
    }

    QCOMPARE(allIds(imported).count(), 3);

    const QHash<QString, QString> ids = idsByPlugin(imported);
    const KConfigGroup movedOuter = dock.group(APPLETS).group(ids.value(QStringLiteral("org.example.outer")));
    const KConfigGroup movedMiddle = movedOuter.group(APPLETS).group(ids.value(QStringLiteral("org.example.middle")));
    QCOMPARE(movedMiddle.group(APPLETS).group(ids.value(QStringLiteral("org.example.inner"))).readEntry(PLUGIN, QString()),
             QStringLiteral("org.example.inner"));
}

void AppletIdsUnitTest::nonNumericGroupsAreIgnored()
{
    KConfigGroup dock = containment(QStringLiteral("12"));
    addApplet(dock, QStringLiteral("40"), QStringLiteral("org.kde.latte.plasmoid"));
    dock.group(APPLETS).group(QStringLiteral("Configuration")).writeEntry(QStringLiteral("key"), QStringLiteral("value"));

    AppletIds::makeUnique({dock}, toIdSet({12, 40}));

    QCOMPARE(dock.group(APPLETS).group(QStringLiteral("Configuration")).readEntry(QStringLiteral("key"), QString()), QStringLiteral("value"));
    QVERIFY(!dock.group(APPLETS).hasGroup(QStringLiteral("40")));
}

QTEST_GUILESS_MAIN(AppletIdsUnitTest)

#include "appletidsunittest.moc"
