/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pluginids.h"
#include "layoutfilelock.h"
#include "layoutfilemove.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>

#include <unistd.h>

// Replicates core logic from app/layouts/storage.cpp; the LayoutFileLock and
// LayoutFileMove tests at the end of this file link the production helpers instead

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

    void layoutMoveChainRenamesEveryFile();
    void layoutMoveSwapExchangesFiles();
    void layoutMoveOfMissingSourceFailsAlone();
    void layoutMoveIntoUnwritableDirectoryKeepsSourceFile();
    void layoutMoveUndoesRoundAndRetriesOthers();
    void layoutMoveRestoresEveryFailedFile();
    void layoutMoveChainBlockedByExistingTargetKeepsEveryFile();

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

namespace {

bool writeLayoutFile(const QString &path, const QByteArray &content)
{
    QFile file(path);

    return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
}

QByteArray readLayoutFile(const QString &path)
{
    QFile file(path);

    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

bool directoryIsEmpty(const QString &path)
{
    return QDir(path).entryList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot).isEmpty();
}

//! Gives the owner write permission of a directory back when it goes out of scope, so that a test
//! which removed it can still remove its temporary directory after a failed check.
class OwnerWriteRestorer
{
public:
    explicit OwnerWriteRestorer(const QString &directory)
        : m_directory(directory)
    {
    }

    ~OwnerWriteRestorer()
    {
        QFile::setPermissions(m_directory, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }

private:
    QString m_directory;
};

}

void StorageLogicTest::layoutMoveChainRenamesEveryFile()
{
    //! Apply renames A to B and B to C: the file of B must leave B before the file of A arrives there
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString tempDir = dir.path() + QStringLiteral("/.apply");
    QVERIFY(QDir().mkpath(tempDir));

    const QString a = dir.path() + QStringLiteral("/A.layout.latte");
    const QString b = dir.path() + QStringLiteral("/B.layout.latte");
    const QString c = dir.path() + QStringLiteral("/C.layout.latte");
    QVERIFY(writeLayoutFile(a, "content of A"));
    QVERIFY(writeLayoutFile(b, "content of B"));

    const auto result = Latte::Layouts::LayoutFileMove::moveFiles({{a, b}, {b, c}}, tempDir);

    QCOMPARE(result.moved, (QList<bool>{true, true}));
    QVERIFY(result.unrecovered.isEmpty());
    QCOMPARE(readLayoutFile(b), QByteArray("content of A"));
    QCOMPARE(readLayoutFile(c), QByteArray("content of B"));
    QVERIFY(!QFileInfo::exists(a));
    QVERIFY(directoryIsEmpty(tempDir));
}

void StorageLogicTest::layoutMoveSwapExchangesFiles()
{
    //! Apply exchanges the names of A and B: each file is parked before the other one takes its path
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString tempDir = dir.path() + QStringLiteral("/.apply");
    QVERIFY(QDir().mkpath(tempDir));

    const QString a = dir.path() + QStringLiteral("/A.layout.latte");
    const QString b = dir.path() + QStringLiteral("/B.layout.latte");
    QVERIFY(writeLayoutFile(a, "content of A"));
    QVERIFY(writeLayoutFile(b, "content of B"));

    const auto result = Latte::Layouts::LayoutFileMove::moveFiles({{a, b}, {b, a}}, tempDir);

    QCOMPARE(result.moved, (QList<bool>{true, true}));
    QVERIFY(result.unrecovered.isEmpty());
    QCOMPARE(readLayoutFile(a), QByteArray("content of B"));
    QCOMPARE(readLayoutFile(b), QByteArray("content of A"));
    QVERIFY(directoryIsEmpty(tempDir));
}

void StorageLogicTest::layoutMoveOfMissingSourceFailsAlone()
{
    //! A layout file that is not on disk cannot be moved and must not be invented at the new path;
    //! the other layout is still moved
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString tempDir = dir.path() + QStringLiteral("/.apply");
    QVERIFY(QDir().mkpath(tempDir));

    const QString ghost = dir.path() + QStringLiteral("/Ghost.layout.latte");
    const QString ghostRenamed = dir.path() + QStringLiteral("/GhostRenamed.layout.latte");
    const QString present = dir.path() + QStringLiteral("/Present.layout.latte");
    const QString presentRenamed = dir.path() + QStringLiteral("/PresentRenamed.layout.latte");
    QVERIFY(writeLayoutFile(present, "content of Present"));

    const auto result = Latte::Layouts::LayoutFileMove::moveFiles({{ghost, ghostRenamed}, {present, presentRenamed}}, tempDir);

    QCOMPARE(result.moved, (QList<bool>{false, true}));
    QVERIFY(result.unrecovered.isEmpty());
    QVERIFY(!QFileInfo::exists(ghostRenamed));
    QCOMPARE(readLayoutFile(presentRenamed), QByteArray("content of Present"));
    QVERIFY(!QFileInfo::exists(present));
    QVERIFY(directoryIsEmpty(tempDir));
}

void StorageLogicTest::layoutMoveIntoUnwritableDirectoryKeepsSourceFile()
{
    //! Root is not bound by directory permissions, so this failure cannot be injected as root
    if (::geteuid() == 0) {
        QSKIP("Root ignores directory permissions; run this test as a normal user");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString tempDir = dir.path() + QStringLiteral("/.apply");
    QVERIFY(QDir().mkpath(tempDir));

    const QString locked = dir.path() + QStringLiteral("/locked");
    QVERIFY(QDir().mkpath(locked));

    const QString work = dir.path() + QStringLiteral("/Work.layout.latte");
    const QString home = dir.path() + QStringLiteral("/Home.layout.latte");
    QVERIFY(writeLayoutFile(work, "content of Work"));
    QVERIFY(writeLayoutFile(home, "content of Home"));

    //! read and search permission only: no file can be created in this directory
    QVERIFY(QFile::setPermissions(locked, QFileDevice::ReadOwner | QFileDevice::ExeOwner));
    const OwnerWriteRestorer restoreLocked(locked);

    const auto result = Latte::Layouts::LayoutFileMove::moveFiles({{work, locked + QStringLiteral("/Office.layout.latte")},
                                                                   {home, dir.path() + QStringLiteral("/Den.layout.latte")}},
                                                                  tempDir);

    QCOMPARE(result.moved, (QList<bool>{false, true}));
    QVERIFY(result.unrecovered.isEmpty());
    //! the layout that could not be placed keeps its file on its old path, the other one is moved
    QCOMPARE(readLayoutFile(work), QByteArray("content of Work"));
    QVERIFY(!QFileInfo::exists(locked + QStringLiteral("/Office.layout.latte")));
    QCOMPARE(readLayoutFile(dir.path() + QStringLiteral("/Den.layout.latte")), QByteArray("content of Home"));
    QVERIFY(!QFileInfo::exists(home));
    QVERIFY(directoryIsEmpty(tempDir));
}

void StorageLogicTest::layoutMoveUndoesRoundAndRetriesOthers()
{
    //! X can be placed and Y cannot, because the target of Y exists. X was placed in the same round, so
    //! that round is undone: Y goes back to its path, and X is placed again in the next round.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString tempDir = dir.path() + QStringLiteral("/.apply");
    QVERIFY(QDir().mkpath(tempDir));

    const QString x = dir.path() + QStringLiteral("/X.layout.latte");
    const QString xRenamed = dir.path() + QStringLiteral("/XRenamed.layout.latte");
    const QString y = dir.path() + QStringLiteral("/Y.layout.latte");
    const QString yRenamed = dir.path() + QStringLiteral("/YRenamed.layout.latte");
    QVERIFY(writeLayoutFile(x, "content of X"));
    QVERIFY(writeLayoutFile(y, "content of Y"));
    QVERIFY(writeLayoutFile(yRenamed, "stray content"));

    const auto result = Latte::Layouts::LayoutFileMove::moveFiles({{x, xRenamed}, {y, yRenamed}}, tempDir);

    QCOMPARE(result.moved, (QList<bool>{true, false}));
    QVERIFY(result.unrecovered.isEmpty());
    QCOMPARE(readLayoutFile(xRenamed), QByteArray("content of X"));
    QVERIFY(!QFileInfo::exists(x));
    QCOMPARE(readLayoutFile(y), QByteArray("content of Y"));
    QCOMPARE(readLayoutFile(yRenamed), QByteArray("stray content"));
    QVERIFY(directoryIsEmpty(tempDir));
}

void StorageLogicTest::layoutMoveRestoresEveryFailedFile()
{
    //! P and R cannot be placed, because their targets exist, and Q is placed between them. Both failed
    //! files go back to their old paths; only Q ends on its new path.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString tempDir = dir.path() + QStringLiteral("/.apply");
    QVERIFY(QDir().mkpath(tempDir));

    const QString p = dir.path() + QStringLiteral("/P.layout.latte");
    const QString pRenamed = dir.path() + QStringLiteral("/PRenamed.layout.latte");
    const QString q = dir.path() + QStringLiteral("/Q.layout.latte");
    const QString qRenamed = dir.path() + QStringLiteral("/QRenamed.layout.latte");
    const QString r = dir.path() + QStringLiteral("/R.layout.latte");
    const QString rRenamed = dir.path() + QStringLiteral("/RRenamed.layout.latte");
    QVERIFY(writeLayoutFile(p, "content of P"));
    QVERIFY(writeLayoutFile(q, "content of Q"));
    QVERIFY(writeLayoutFile(r, "content of R"));
    QVERIFY(writeLayoutFile(pRenamed, "stray P"));
    QVERIFY(writeLayoutFile(rRenamed, "stray R"));

    const auto result = Latte::Layouts::LayoutFileMove::moveFiles({{p, pRenamed}, {q, qRenamed}, {r, rRenamed}}, tempDir);

    QCOMPARE(result.moved, (QList<bool>{false, true, false}));
    QVERIFY(result.unrecovered.isEmpty());
    QCOMPARE(readLayoutFile(p), QByteArray("content of P"));
    QCOMPARE(readLayoutFile(r), QByteArray("content of R"));
    QVERIFY(!QFileInfo::exists(q));
    QCOMPARE(readLayoutFile(qRenamed), QByteArray("content of Q"));
    QCOMPARE(readLayoutFile(pRenamed), QByteArray("stray P"));
    QCOMPARE(readLayoutFile(rRenamed), QByteArray("stray R"));
    QVERIFY(directoryIsEmpty(tempDir));
}

void StorageLogicTest::layoutMoveChainBlockedByExistingTargetKeepsEveryFile()
{
    //! A is renamed to B and B to C, but C exists: B cannot move, so A cannot take the name of B either
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString tempDir = dir.path() + QStringLiteral("/.apply");
    QVERIFY(QDir().mkpath(tempDir));

    const QString a = dir.path() + QStringLiteral("/A.layout.latte");
    const QString b = dir.path() + QStringLiteral("/B.layout.latte");
    const QString c = dir.path() + QStringLiteral("/C.layout.latte");
    QVERIFY(writeLayoutFile(a, "content of A"));
    QVERIFY(writeLayoutFile(b, "content of B"));
    QVERIFY(writeLayoutFile(c, "stray content"));

    const auto result = Latte::Layouts::LayoutFileMove::moveFiles({{a, b}, {b, c}}, tempDir);

    QCOMPARE(result.moved, (QList<bool>{false, false}));
    QVERIFY(result.unrecovered.isEmpty());
    QCOMPARE(readLayoutFile(a), QByteArray("content of A"));
    QCOMPARE(readLayoutFile(b), QByteArray("content of B"));
    QCOMPARE(readLayoutFile(c), QByteArray("stray content"));
    QVERIFY(directoryIsEmpty(tempDir));
}

QTEST_MAIN(StorageLogicTest)
#include "storagelogictest.moc"
