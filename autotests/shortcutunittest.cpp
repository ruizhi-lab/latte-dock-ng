/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "modifiertracker.h"
#include "shortcutstracker.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class ShortcutsUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void modifierTrackerStartsWithNoPressedModifiers();
    void metaHoldFiresForEveryKeyThePlatformReportsForMeta();
    void metaTapShorterThanTheHoldDoesNotFire();
    void metaHoldWithAnotherModifierDoesNotFire();
    void blockedMetaTrackingIsIgnoredUntilUnblocked();
    void shortcutsTrackerParsesActivateBadgesFromConfig();
};

void ShortcutsUnitTest::modifierTrackerStartsWithNoPressedModifiers()
{
    Latte::ShortcutsPart::ModifierTracker tracker(this);

    QVERIFY(tracker.noModifierPressed());
    QVERIFY(!tracker.sequenceModifierPressed(QKeySequence()));
    QVERIFY(!tracker.sequenceModifierPressed(QKeySequence(QKeyCombination(Qt::ControlModifier, Qt::Key_1))));
    QVERIFY(!tracker.singleModifierPressed(Qt::Key_Control));

    tracker.blockModifierTracking(Qt::Key_Control);
    tracker.blockModifierTracking(Qt::Key_Control);
    tracker.unblockModifierTracking(Qt::Key_Control);
    QVERIFY(tracker.noModifierPressed());
}

void ShortcutsUnitTest::metaHoldFiresForEveryKeyThePlatformReportsForMeta()
{
    // KModifierKeyInfo reports Meta as Key_Meta on Wayland (this is a Wayland-only build) and as
    // Key_Super_L or Key_Super_R on X11. Before Key_Meta was accepted the press was dropped and
    // holding Meta never showed the docks.
    for (const Qt::Key key : {Qt::Key_Meta, Qt::Key_Super_L, Qt::Key_Super_R}) {
        Latte::ShortcutsPart::ModifierTracker tracker(this);
        QSignalSpy holdSpy(&tracker, &Latte::ShortcutsPart::ModifierTracker::metaModifierPressed);

        tracker.handleKeyPressed(key, true);
        QVERIFY(!tracker.noModifierPressed());
        QVERIFY(tracker.singleModifierPressed(Qt::Key_Super_L));
        QCOMPARE(holdSpy.count(), 0);

        // the hold time is 700 ms
        QVERIFY2(holdSpy.wait(2000), qPrintable(QStringLiteral("no hold for key %1").arg(static_cast<int>(key))));
        QCOMPARE(holdSpy.count(), 1);

        tracker.handleKeyPressed(key, false);
        QVERIFY(tracker.noModifierPressed());
    }
}

void ShortcutsUnitTest::metaTapShorterThanTheHoldDoesNotFire()
{
    Latte::ShortcutsPart::ModifierTracker tracker(this);
    QSignalSpy holdSpy(&tracker, &Latte::ShortcutsPart::ModifierTracker::metaModifierPressed);

    tracker.handleKeyPressed(Qt::Key_Meta, true);
    tracker.handleKeyPressed(Qt::Key_Meta, false);

    QVERIFY(tracker.noModifierPressed());
    QVERIFY(!holdSpy.wait(1000));
    QCOMPARE(holdSpy.count(), 0);
}

void ShortcutsUnitTest::metaHoldWithAnotherModifierDoesNotFire()
{
    Latte::ShortcutsPart::ModifierTracker tracker(this);
    QSignalSpy holdSpy(&tracker, &Latte::ShortcutsPart::ModifierTracker::metaModifierPressed);

    // another modifier is already down: Meta is not pressed alone
    tracker.handleKeyPressed(Qt::Key_Shift, true);
    tracker.handleKeyPressed(Qt::Key_Meta, true);
    QVERIFY(!tracker.singleModifierPressed(Qt::Key_Super_L));
    QVERIFY(!holdSpy.wait(1000));
    tracker.handleKeyPressed(Qt::Key_Meta, false);
    tracker.handleKeyPressed(Qt::Key_Shift, false);
    QVERIFY(tracker.noModifierPressed());

    // another modifier joins a pending hold: it is cancelled
    tracker.handleKeyPressed(Qt::Key_Meta, true);
    tracker.handleKeyPressed(Qt::Key_Control, true);
    QVERIFY(!holdSpy.wait(1000));
    QCOMPARE(holdSpy.count(), 0);
}

void ShortcutsUnitTest::blockedMetaTrackingIsIgnoredUntilUnblocked()
{
    Latte::ShortcutsPart::ModifierTracker tracker(this);
    QSignalSpy holdSpy(&tracker, &Latte::ShortcutsPart::ModifierTracker::metaModifierPressed);

    // GlobalShortcuts blocks both Super keys while the Preferences option is off
    tracker.blockModifierTracking(Qt::Key_Super_L);
    tracker.blockModifierTracking(Qt::Key_Super_R);

    tracker.handleKeyPressed(Qt::Key_Meta, true);
    QVERIFY(tracker.noModifierPressed());
    QVERIFY(!holdSpy.wait(1000));
    tracker.handleKeyPressed(Qt::Key_Meta, false);

    tracker.unblockModifierTracking(Qt::Key_Super_L);
    tracker.unblockModifierTracking(Qt::Key_Super_R);

    tracker.handleKeyPressed(Qt::Key_Meta, true);
    QVERIFY(holdSpy.wait(2000));
    QCOMPARE(holdSpy.count(), 1);
}

void ShortcutsUnitTest::shortcutsTrackerParsesActivateBadgesFromConfig()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    qputenv("XDG_CONFIG_HOME", dir.path().toUtf8());

    const QString configPath = dir.path() + QStringLiteral("/kglobalshortcutsrc");
    KSharedConfig::Ptr config = KSharedConfig::openConfig(configPath);
    KConfigGroup latteGroup(config, QStringLiteral("lattedock"));
    latteGroup.writeEntry(QStringLiteral("activate entry 1"), QStringList{QStringLiteral("Meta+1")});
    latteGroup.writeEntry(QStringLiteral("activate entry 2"), QStringList{QStringLiteral("Meta+A")});
    latteGroup.writeEntry(QStringLiteral("activate entry 3"), QStringList{QStringLiteral("Ctrl+Alt+B")});
    latteGroup.sync();
    config->sync();

    Latte::ShortcutsPart::ShortcutsTracker tracker(this);
    QSignalSpy changedSpy(&tracker, &Latte::ShortcutsPart::ShortcutsTracker::badgesForActivateChanged);

    QCOMPARE(tracker.badgesForActivate().count(), 19);
    QCOMPARE(tracker.badgesForActivate().at(0), QStringLiteral("1"));
    QCOMPARE(tracker.badgesForActivate().at(1), QStringLiteral("a"));
    QCOMPARE(tracker.badgesForActivate().at(2), QStringLiteral("B"));
    QVERIFY(tracker.basedOnPositionEnabled());
    QVERIFY(tracker.appletsWithPlasmaShortcuts().isEmpty());
    QCOMPARE(tracker.appletShortcutBadge(123), QString());
    QCOMPARE(changedSpy.count(), 0);
}

QTEST_GUILESS_MAIN(ShortcutsUnitTest)

#include "shortcutunittest.moc"
