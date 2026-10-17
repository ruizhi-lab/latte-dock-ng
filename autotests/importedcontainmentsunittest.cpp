/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../app/layouts/importedcontainments.h"
#include "../app/plasma/plasmaapicompat.h"

#include <KConfigGroup>
#include <KPluginMetaData>

#include <Plasma/Containment>
#include <Plasma/Corona>

#include <QJsonObject>
#include <QPointer>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

using namespace Latte::Layouts;

namespace {

class TestCorona : public Plasma::Corona
{
public:
    using Plasma::Corona::Corona;

    QRect screenGeometry(Latte::PlasmaScreenId id) const override
    {
        Q_UNUSED(id)
        return {};
    }
};

//! libplasma before 6.8 warns about every containment without a type.
KPluginMetaData testContainmentMetaData()
{
    const QJsonObject plugin{{QStringLiteral("Id"), QStringLiteral("org.kde.latte.test.containment")}};
    return KPluginMetaData(QJsonObject{{QStringLiteral("KPlugin"), plugin}, {QStringLiteral("X-Plasma-ContainmentType"), QStringLiteral("Custom")}}, QString());
}

//! A dock whose system tray applet can migrate a legacy tray containment:
//! restoring it deletes that containment synchronously, as libplasma 6.8's
//! SystemTray::migrateFromSystrayContainer() does.
class TestContainment : public Plasma::Containment
{
public:
    TestContainment(Plasma::Corona *corona, uint id)
        : Plasma::Containment(corona, testContainmentMetaData(), QVariantList{QVariant(), id})
    {
    }

    void migrateOnRestore(Plasma::Containment *legacyTray)
    {
        m_legacyTray = legacyTray;
    }

    int restoreCount() const
    {
        return m_restoreCount;
    }

    void restore(KConfigGroup &group) override
    {
        Plasma::Containment::restore(group);
        ++m_restoreCount;
        delete m_legacyTray.data();
    }

private:
    QPointer<Plasma::Containment> m_legacyTray;
    int m_restoreCount{0};
};

QList<Plasma::Containment *> announced(const QSignalSpy &spy)
{
    QList<Plasma::Containment *> result;

    for (const auto &arguments : spy) {
        result << arguments.at(0).value<Plasma::Containment *>();
    }

    return result;
}

}

class ImportedContainmentsUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void restoresInOrderAndReturnsAll();
    void legacyTrayRestoredBeforeItsHostIsNotReturned();
    void legacyTrayDeletedBeforeItsOwnRestoreIsSkipped();
};

void ImportedContainmentsUnitTest::initTestCase()
{
    //! Plasma::Corona keeps its configuration in <app>-appletsrc.
    QStandardPaths::setTestModeEnabled(true);
}

void ImportedContainmentsUnitTest::restoresInOrderAndReturnsAll()
{
    TestCorona corona;
    auto *dock = new TestContainment(&corona, 19);
    auto *panel = new TestContainment(&corona, 20);
    QSignalSpy added(&corona, &Plasma::Corona::containmentAdded);

    const auto restored = ImportedContainments::restore(&corona, {dock, panel});

    QCOMPARE(restored, (QList<Plasma::Containment *>{dock, panel}));
    QCOMPARE(announced(added), (QList<Plasma::Containment *>{dock, panel}));
    QCOMPARE(dock->restoreCount(), 1);
    QCOMPARE(panel->restoreCount(), 1);

    qDeleteAll(restored);
}

void ImportedContainmentsUnitTest::legacyTrayRestoredBeforeItsHostIsNotReturned()
{
    //! The import restores containments in the string order of their ids:
    //! the tray "100" comes before its dock "19", is fully restored and is
    //! only deleted when the dock is restored.
    TestCorona corona;
    QPointer<Plasma::Containment> tray = new TestContainment(&corona, 100);
    auto *dock = new TestContainment(&corona, 19);
    dock->migrateOnRestore(tray);
    QSignalSpy added(&corona, &Plasma::Corona::containmentAdded);

    const auto restored = ImportedContainments::restore(&corona, {tray, dock});

    QVERIFY(tray.isNull());
    QCOMPARE(added.count(), 2);
    QCOMPARE(restored, QList<Plasma::Containment *>{dock});
    QCOMPARE(restored.constFirst()->id(), 19U);

    qDeleteAll(restored);
}

void ImportedContainmentsUnitTest::legacyTrayDeletedBeforeItsOwnRestoreIsSkipped()
{
    //! The dock "19" comes before its tray "20": the tray is deleted while it
    //! is still waiting for its own restore.
    TestCorona corona;
    auto *dock = new TestContainment(&corona, 19);
    QPointer<Plasma::Containment> tray = new TestContainment(&corona, 20);
    dock->migrateOnRestore(tray);
    QSignalSpy added(&corona, &Plasma::Corona::containmentAdded);

    const auto restored = ImportedContainments::restore(&corona, {dock, tray});

    QVERIFY(tray.isNull());
    QCOMPARE(announced(added), QList<Plasma::Containment *>{dock});
    QCOMPARE(restored, QList<Plasma::Containment *>{dock});

    qDeleteAll(restored);
}

QTEST_MAIN(ImportedContainmentsUnitTest)

#include "importedcontainmentsunittest.moc"
