/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pluginids.h"
#include "layoutfilelock.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>

// Replicates core logic from app/layouts/storage.cpp; the LayoutFileLock tests
// at the end of this file link the production helper instead

namespace StorageLogic {

constexpr int IDNULL = -1;
constexpr int IDBASE = 0;

bool isValid(int id) { return id >= IDBASE; }

bool appletGroupIsValid(const KConfigGroup &g)
{
    return !(g.keyList().count() == 0 && g.groupList().count() == 1
             && g.groupList().at(0) == QLatin1String("Configuration")
             && g.group(QStringLiteral("Configuration")).keyList().count() == 1
             && g.group(QStringLiteral("Configuration")).hasKey("PreloadWeight"));
}

bool isLatteContainmentGroup(const KConfigGroup &g)
{
    return g.readEntry(QStringLiteral("plugin"), QString()) == QLatin1String(Latte::PluginId::kContainment);
}

QStringList containmentIds(KSharedConfigPtr c)
{
    QStringList ids; KConfigGroup g(c, QStringLiteral("Containments"));

    for (const auto &id : g.groupList()) ids << id;

    return ids;
}

QStringList appletIds(KSharedConfigPtr c)
{
    QStringList ids; KConfigGroup g(c, QStringLiteral("Containments"));

    for (const auto &cid : g.groupList()) {
        KConfigGroup cg = g.group(cid).group(QStringLiteral("Applets"));

        for (const auto &aid : cg.groupList()) ids << aid;
    }

    return ids;
}

QString availableId(const QStringList &all, const QStringList &assigned, int base)
{
    int i = base;

    while (i < 32000) {
        const QString s = QString::number(i);

        if (!all.contains(s) && !assigned.contains(s)) return s;

        i++;
    }

    return {};
}

} // namespace StorageLogic

class StorageLogicTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void idZeroAndPositiveAreValid();
    void negativeIdsAreInvalid();
    void idNullAndBaseConstants();

    void normalAppletGroupIsValid();
    void emptyShellWithPreloadWeightIsInvalid();
    void groupWithKeysOrMultipleSubgroupsIsValid();
    void completelyEmptyGroupIsValid();

    void latteContainmentPluginRecognized();
    void nonLattePluginsNotRecognized();
    void emptyPluginEntryNotRecognized();

    void containmentAndAppletIdExtraction();
    void containmentWithoutAppletsReturnsNoAppletIds();

    void availableIdAtBase();
    void availableIdSkipsUsedAndAssigned();
    void availableIdEmptyWhenExhausted();

    void separatorPluginIdsRecognized();
    void latteBuiltInsRecognized();
    void nonBuiltInsNotRecognized();

    void layoutFileWithContainmentsAndApplets();
    void idConflictsAreDetectable();

    void renamedLockedLayoutIsLockedAtItsNewPath();
    void lockOfMissingLayoutFileChangesNothing();

private:
    QTemporaryDir m_tmp;
};

void StorageLogicTest::idZeroAndPositiveAreValid()
{
    QVERIFY(StorageLogic::isValid(0)); QVERIFY(StorageLogic::isValid(42));
}

void StorageLogicTest::negativeIdsAreInvalid() { QVERIFY(!StorageLogic::isValid(-1)); }

void StorageLogicTest::idNullAndBaseConstants()
{
    QCOMPARE(StorageLogic::IDNULL, -1); QCOMPARE(StorageLogic::IDBASE, 0);
}

void StorageLogicTest::normalAppletGroupIsValid()
{
    QTemporaryFile f(m_tmp.path() + QLatin1String("/t")); QVERIFY(f.open()); f.close();
    auto c = KSharedConfig::openConfig(f.fileName());
    KConfigGroup g(c, QStringLiteral("Applets")); g.group(QStringLiteral("42")).group(QStringLiteral("Configuration")).writeEntry(QStringLiteral("plugin"), QStringLiteral("x"));
    QVERIFY(StorageLogic::appletGroupIsValid(g.group(QStringLiteral("42"))));
}

void StorageLogicTest::emptyShellWithPreloadWeightIsInvalid()
{
    QTemporaryFile f(m_tmp.path() + QLatin1String("/t")); QVERIFY(f.open()); f.close();
    auto c = KSharedConfig::openConfig(f.fileName());
    KConfigGroup g(c, QStringLiteral("Applets")); g.group(QStringLiteral("99")).group(QStringLiteral("Configuration")).writeEntry(QStringLiteral("PreloadWeight"), 42);
    QVERIFY(!StorageLogic::appletGroupIsValid(g.group(QStringLiteral("99"))));
}

void StorageLogicTest::groupWithKeysOrMultipleSubgroupsIsValid()
{
    QTemporaryFile f(m_tmp.path() + QLatin1String("/t")); QVERIFY(f.open()); f.close();
    auto c = KSharedConfig::openConfig(f.fileName());
    KConfigGroup g(c, QStringLiteral("Applets")); g.group(QStringLiteral("1")).writeEntry(QStringLiteral("immutability"), 1);
    QVERIFY(StorageLogic::appletGroupIsValid(g.group(QStringLiteral("1"))));
}

void StorageLogicTest::completelyEmptyGroupIsValid()
{
    QTemporaryFile f(m_tmp.path() + QLatin1String("/t")); QVERIFY(f.open()); f.close();
    auto c = KSharedConfig::openConfig(f.fileName());
    KConfigGroup g(c, QStringLiteral("Applets")); g.group(QStringLiteral("77"));
    QVERIFY(StorageLogic::appletGroupIsValid(g.group(QStringLiteral("77"))));
}

void StorageLogicTest::latteContainmentPluginRecognized()
{
    QTemporaryFile f(m_tmp.path() + QLatin1String("/t")); QVERIFY(f.open()); f.close();
    auto c = KSharedConfig::openConfig(f.fileName());
    KConfigGroup g(c, QStringLiteral("Containments")); g.group(QStringLiteral("1")).writeEntry(QStringLiteral("plugin"), Latte::PluginId::kContainment);
    QVERIFY(StorageLogic::isLatteContainmentGroup(g.group(QStringLiteral("1"))));
}

void StorageLogicTest::nonLattePluginsNotRecognized()
{
    QTemporaryFile f(m_tmp.path() + QLatin1String("/t")); QVERIFY(f.open()); f.close();
    auto c = KSharedConfig::openConfig(f.fileName());
    KConfigGroup g(c, QStringLiteral("Containments")); g.group(QStringLiteral("1")).writeEntry(QStringLiteral("plugin"), "org.kde.panel");
    QVERIFY(!StorageLogic::isLatteContainmentGroup(g.group(QStringLiteral("1"))));
}

void StorageLogicTest::emptyPluginEntryNotRecognized()
{
    QTemporaryFile f(m_tmp.path() + QLatin1String("/t")); QVERIFY(f.open()); f.close();
    auto c = KSharedConfig::openConfig(f.fileName());
    KConfigGroup g(c, QStringLiteral("Containments")); g.group(QStringLiteral("1"));
    QVERIFY(!StorageLogic::isLatteContainmentGroup(g.group(QStringLiteral("1"))));
}

void StorageLogicTest::containmentAndAppletIdExtraction()
{
    QTemporaryFile f(m_tmp.path() + QLatin1String("/t")); QVERIFY(f.open()); f.close();
    auto c = KSharedConfig::openConfig(f.fileName());
    KConfigGroup g(c, QStringLiteral("Containments"));
    g.group(QStringLiteral("12")).writeEntry(QStringLiteral("plugin"), QStringLiteral("x"));
    g.group(QStringLiteral("12")).group(QStringLiteral("Applets")).group(QStringLiteral("30")).writeEntry(QStringLiteral("plugin"), "a");
    g.group(QStringLiteral("12")).group(QStringLiteral("Applets")).group(QStringLiteral("31")).writeEntry(QStringLiteral("plugin"), "b");
    g.group(QStringLiteral("42")).writeEntry(QStringLiteral("plugin"), "y");
    c->sync();
    QCOMPARE(StorageLogic::containmentIds(c).size(), 2);
    QCOMPARE(StorageLogic::appletIds(c).size(), 2);
}

void StorageLogicTest::containmentWithoutAppletsReturnsNoAppletIds()
{
    QTemporaryFile f(m_tmp.path() + QLatin1String("/t")); QVERIFY(f.open()); f.close();
    auto c = KSharedConfig::openConfig(f.fileName());
    KConfigGroup g(c, QStringLiteral("Containments")); g.group(QStringLiteral("42")).writeEntry(QStringLiteral("plugin"), QStringLiteral("x")); c->sync();
    QVERIFY(StorageLogic::appletIds(c).isEmpty());
}

void StorageLogicTest::availableIdAtBase()
{
    QCOMPARE(StorageLogic::availableId({}, {}, 0), QStringLiteral("0"));
}

void StorageLogicTest::availableIdSkipsUsedAndAssigned()
{
    QCOMPARE(StorageLogic::availableId({QStringLiteral("0")}, {QStringLiteral("1")}, 0), QStringLiteral("2"));
}

void StorageLogicTest::availableIdEmptyWhenExhausted()
{
    QCOMPARE(StorageLogic::availableId({QStringLiteral("31999")}, {}, 31999), QString());
}

void StorageLogicTest::separatorPluginIdsRecognized()
{
    QVERIFY(Latte::PluginId::isSeparatorPluginId(QString::fromLatin1(Latte::PluginId::kSeparator)));
    QVERIFY(Latte::PluginId::isSeparatorPluginId(QString::fromLatin1(Latte::PluginId::kLegacySeparator)));
    QVERIFY(!Latte::PluginId::isSeparatorPluginId(QString::fromLatin1(Latte::PluginId::kSpacer)));
}

void StorageLogicTest::latteBuiltInsRecognized()
{
    QVERIFY(Latte::PluginId::isLatteBuiltIn(QString::fromLatin1(Latte::PluginId::kSpacer)));
    QVERIFY(Latte::PluginId::isLatteBuiltIn(QString::fromLatin1(Latte::PluginId::kPlasmoid)));
    QVERIFY(Latte::PluginId::isLatteBuiltIn(QString::fromLatin1(Latte::PluginId::kSplitter)));
}

void StorageLogicTest::nonBuiltInsNotRecognized()
{
    QVERIFY(!Latte::PluginId::isLatteBuiltIn(QString::fromLatin1(Latte::PluginId::kContainment)));
    QVERIFY(!Latte::PluginId::isLatteBuiltIn(QString()));
}

void StorageLogicTest::layoutFileWithContainmentsAndApplets()
{
    QTemporaryFile f(m_tmp.path() + QLatin1String("/t")); QVERIFY(f.open()); f.close();
    auto c = KSharedConfig::openConfig(f.fileName());
    KConfigGroup g(c, QStringLiteral("Containments"));
    KConfigGroup c12 = g.group(QStringLiteral("12")); c12.writeEntry(QStringLiteral("plugin"), Latte::PluginId::kContainment);
    c12.group(QStringLiteral("Applets")).group(QStringLiteral("30")).writeEntry(QStringLiteral("plugin"), "org.kde.plasma.analogclock");
    KConfigGroup c42 = g.group(QStringLiteral("42")); c42.writeEntry(QStringLiteral("plugin"), Latte::PluginId::kContainment);
    c42.group(QStringLiteral("Applets")).group(QStringLiteral("50")).writeEntry(QStringLiteral("plugin"), Latte::PluginId::kSpacer);
    c->sync();
    QCOMPARE(StorageLogic::containmentIds(c).size(), 2);
    QCOMPARE(StorageLogic::appletIds(c).size(), 2);
    QVERIFY(StorageLogic::isLatteContainmentGroup(g.group(QStringLiteral("12"))));
    QVERIFY(StorageLogic::isLatteContainmentGroup(g.group(QStringLiteral("42"))));
}

void StorageLogicTest::idConflictsAreDetectable()
{
    QStringList cids{QStringLiteral("1"), QStringLiteral("12"), QStringLiteral("30")};
    QStringList aids{QStringLiteral("30"), QStringLiteral("31")};
    bool conflict = false;

    for (const auto &id : cids) { if (aids.contains(id)) { conflict = true; break; } }

    QVERIFY(conflict);
    cids.removeAll(QStringLiteral("30"));
    conflict = false;

    for (const auto &id : cids) { if (aids.contains(id)) { conflict = true; break; } }

    QVERIFY(!conflict);
}

namespace {

//! The owner write bit is what lock() and unlock() change. isWritable() is not used
//! here: access(2) reports read-only files as writable for root.
bool ownerCanWrite(const QString &file)
{
    return QFileInfo(file).permissions().testFlag(QFileDevice::WriteUser);
}

}

void StorageLogicTest::renamedLockedLayoutIsLockedAtItsNewPath()
{
    //! Apply unlocks a locked layout before it renames the file, then locks the
    //! file again at the path it has after the rename. Locking the old path does
    //! not restore the lock: after the rename that path has no file.
    const QString oldFile = m_tmp.path() + QStringLiteral("/Work.layout.latte");
    const QString newFile = m_tmp.path() + QStringLiteral("/Office.layout.latte");

    QFile file(oldFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("[LayoutSettings]\n");
    file.close();

    Latte::Layouts::LayoutFileLock::lock(oldFile);
    QVERIFY(!ownerCanWrite(oldFile));

    Latte::Layouts::LayoutFileLock::unlock(oldFile);
    QVERIFY(ownerCanWrite(oldFile));
    QVERIFY(QFile::rename(oldFile, newFile));

    Latte::Layouts::LayoutFileLock::lock(oldFile);
    QVERIFY(!QFileInfo::exists(oldFile));
    QVERIFY(ownerCanWrite(newFile));

    Latte::Layouts::LayoutFileLock::lock(newFile);
    QVERIFY(!ownerCanWrite(newFile));

    Latte::Layouts::LayoutFileLock::unlock(newFile);
    QVERIFY(ownerCanWrite(newFile));
}

void StorageLogicTest::lockOfMissingLayoutFileChangesNothing()
{
    const QString missingFile = m_tmp.path() + QStringLiteral("/Missing.layout.latte");

    Latte::Layouts::LayoutFileLock::lock(missingFile);
    Latte::Layouts::LayoutFileLock::unlock(missingFile);

    QVERIFY(!QFileInfo::exists(missingFile));
}

QTEST_MAIN(StorageLogicTest)
#include "storagelogictest.moc"
