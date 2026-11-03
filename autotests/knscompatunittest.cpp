/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "knscompat.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

class KnsCompatUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();
    void latestStampStillRepairsMissingControlsOverride();
    void firstRunWritesPatchedOverridesAndStamp();
    void optOutDoesNotWriteOverrides();
    void usesConfiguredSystemQmlRoot();
    void ignoresRelativeConfiguredSystemQmlRoot();
    void usesConfiguredUserQmlRoot();
    void ignoresRelativeConfiguredUserQmlRoot();
    void replacesSymlinkedPatchedFilesWithoutTouchingTarget();
    void missingSystemQmlRootDoesNotCreatePartialOverrides();
    void systemTemplatesQmldirMirrorsNewTypes();
    void systemTemplatesQmldirDropsPluginDirectives();

private:
    void isolateHomeAndData(QTemporaryDir &home, QTemporaryDir &data);
    QString qmlRoot() const;
    void createSystemQmlRoot(const QString &root) const;
};

void KnsCompatUnitTest::init()
{
    qunsetenv("LATTE_DISABLE_KNS_COMPAT");
    qunsetenv("LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS");
    qunsetenv("LATTE_KNS_COMPAT_USER_QML_ROOT");
}

void KnsCompatUnitTest::cleanup()
{
    qunsetenv("LATTE_DISABLE_KNS_COMPAT");
    qunsetenv("LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS");
    qunsetenv("LATTE_KNS_COMPAT_USER_QML_ROOT");
    QDir(QStringLiteral("relative-system-qml")).removeRecursively();
    QDir(QStringLiteral("relative-qml-root")).removeRecursively();
}

void KnsCompatUnitTest::isolateHomeAndData(QTemporaryDir &home, QTemporaryDir &data)
{
    QVERIFY(home.isValid());
    QVERIFY(data.isValid());

    qputenv("HOME", QFile::encodeName(home.path()));
    qputenv("XDG_DATA_HOME", QFile::encodeName(data.path()));
    QStandardPaths::setTestModeEnabled(false);
}

QString KnsCompatUnitTest::qmlRoot() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + QStringLiteral("/latte-dock-ng/qml-kns-compat");
}

void KnsCompatUnitTest::createSystemQmlRoot(const QString &root) const
{
    const QString templates = root + QStringLiteral("/org/kde/kirigami/templates");
    const QString newstuff = root + QStringLiteral("/org/kde/newstuff");
    const QString controls = root + QStringLiteral("/org/kde/kirigami/controls");

    QVERIFY(QDir().mkpath(templates + QStringLiteral("/private")));
    QVERIFY(QDir().mkpath(newstuff + QStringLiteral("/private/actions")));
    QVERIFY(QDir().mkpath(controls + QStringLiteral("/private/globaltoolbar")));

    QFile templatesQml(templates + QStringLiteral("/Heading.qml"));
    QVERIFY(templatesQml.open(QFile::WriteOnly | QFile::Truncate));
    templatesQml.write("import QtQuick\nItem {}\n");
    templatesQml.close();

    QFile templatesPrivate(templates + QStringLiteral("/private/OverlayDrawerHandle.qml"));
    QVERIFY(templatesPrivate.open(QFile::WriteOnly | QFile::Truncate));
    templatesPrivate.write("import QtQuick\nItem {}\n");
    templatesPrivate.close();

    QFile newstuffQml(newstuff + QStringLiteral("/Button.qml"));
    QVERIFY(newstuffQml.open(QFile::WriteOnly | QFile::Truncate));
    newstuffQml.write("import QtQuick\nItem {}\n");
    newstuffQml.close();

    QFile newstuffPlugin(newstuff + QStringLiteral("/libnewstuffqmlplugin.so"));
    QVERIFY(newstuffPlugin.open(QFile::WriteOnly | QFile::Truncate));
    newstuffPlugin.close();

    QFile newstuffVersion(newstuff + QStringLiteral("/kde-qmlmodule.version"));
    QVERIFY(newstuffVersion.open(QFile::WriteOnly | QFile::Truncate));
    newstuffVersion.close();

    QFile newstuffPrivate(newstuff + QStringLiteral("/private/actions/Action.qml"));
    QVERIFY(newstuffPrivate.open(QFile::WriteOnly | QFile::Truncate));
    newstuffPrivate.write("import QtQuick\nItem {}\n");
    newstuffPrivate.close();

    QFile controlsQmldir(controls + QStringLiteral("/qmldir"));
    QVERIFY(controlsQmldir.open(QFile::WriteOnly | QFile::Truncate));
    controlsQmldir.write("module org.kde.kirigami.controls\nprefer :/qt/qml/org/kde/kirigami/controls\n");
    controlsQmldir.close();

    QFile controlsQml(controls + QStringLiteral("/ActionToolBar.qml"));
    QVERIFY(controlsQml.open(QFile::WriteOnly | QFile::Truncate));
    controlsQml.write("import QtQuick\nItem {}\n");
    controlsQml.close();

    QFile controlsPlugin(controls + QStringLiteral("/libKirigamiControlsplugin.so"));
    QVERIFY(controlsPlugin.open(QFile::WriteOnly | QFile::Truncate));
    controlsPlugin.close();

    QFile controlsVersion(controls + QStringLiteral("/kde-qmlmodule.version"));
    QVERIFY(controlsVersion.open(QFile::WriteOnly | QFile::Truncate));
    controlsVersion.close();

    QFile controlsPrivate(controls + QStringLiteral("/private/globaltoolbar/OtherButton.qml"));
    QVERIFY(controlsPrivate.open(QFile::WriteOnly | QFile::Truncate));
    controlsPrivate.write("import QtQuick\nItem {}\n");
    controlsPrivate.close();
}

void KnsCompatUnitTest::latestStampStillRepairsMissingControlsOverride()
{
    QTemporaryDir home;
    QTemporaryDir data;
    isolateHomeAndData(home, data);

    const QString stampDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                             + QStringLiteral("/latte-dock-ng");
    QVERIFY(QDir().mkpath(stampDir));
    QFile stamp(stampDir + QStringLiteral("/kns-compat.stamp"));
    QVERIFY(stamp.open(QFile::WriteOnly | QFile::Truncate));
    QCOMPARE(stamp.write("8"), qint64(1));
    stamp.close();

    const QString root = qmlRoot();
    QVERIFY(QDir().mkpath(root + QStringLiteral("/org/kde/kirigami/templates")));
    QVERIFY(QDir().mkpath(root + QStringLiteral("/org/kde/newstuff")));

    QFile templatesQmldir(root + QStringLiteral("/org/kde/kirigami/templates/qmldir"));
    QVERIFY(templatesQmldir.open(QFile::WriteOnly | QFile::Truncate));
    templatesQmldir.close();

    QFile newstuffQmldir(root + QStringLiteral("/org/kde/newstuff/qmldir"));
    QVERIFY(newstuffQmldir.open(QFile::WriteOnly | QFile::Truncate));
    newstuffQmldir.close();

    ensureKnsCompat();

    //! The templates and newstuff overrides must always be repaired. The
    //! controls override only exists when the system still ships the legacy
    //! standalone org.kde.kirigami.controls module; Kirigami >= 6.12 merged
    //! it into the main module and nothing is created for it.
    QVERIFY(QFile::exists(root + QStringLiteral("/org/kde/kirigami/templates/qmldir")));
    QVERIFY(QFile::exists(root + QStringLiteral("/org/kde/newstuff/qmldir")));

    const QString systemControls = QLibraryInfo::path(QLibraryInfo::QmlImportsPath)
                                   + QStringLiteral("/org/kde/kirigami/controls/qmldir");

    if (QFile::exists(systemControls)) {
        QVERIFY(QFile::exists(root + QStringLiteral("/org/kde/kirigami/controls/qmldir")));
        QVERIFY(QFile::exists(root + QStringLiteral("/org/kde/kirigami/controls/private/globaltoolbar/HandleButton.qml")));
    } else {
        QVERIFY(!QFile::exists(root + QStringLiteral("/org/kde/kirigami/controls/qmldir")));
    }
}

void KnsCompatUnitTest::firstRunWritesPatchedOverridesAndStamp()
{
    QTemporaryDir home;
    QTemporaryDir data;
    QTemporaryDir systemQml;
    isolateHomeAndData(home, data);
    QVERIFY(systemQml.isValid());
    createSystemQmlRoot(systemQml.path());
    qputenv("LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS", QFile::encodeName(systemQml.path()));

    ensureKnsCompat();

    const QString root = qmlRoot();
    QFile drawer(root + QStringLiteral("/org/kde/kirigami/templates/private/DrawerHandle.qml"));
    QVERIFY(drawer.open(QFile::ReadOnly));
    const QByteArray drawerText = drawer.readAll();
    QVERIFY(drawerText.contains("onActiveTranslationChanged"));
    QVERIFY(drawerText.contains("parent?.width ?? 0"));
    QVERIFY(drawerText.contains("handleAnchor || !parent ? undefined : parent.bottom"));
    QVERIFY(!QFileInfo(drawer).isSymLink());

    QFile handle(root + QStringLiteral("/org/kde/kirigami/controls/private/globaltoolbar/HandleButton.qml"));
    QVERIFY(handle.open(QFile::ReadOnly));
    const QByteArray handleText = handle.readAll();
    QVERIFY(handleText.contains("onActiveTranslationChanged"));
    QVERIFY(handleText.contains("parent?.width ?? 0"));
    QVERIFY(!QFileInfo(handle).isSymLink());

    QFile controlsQmldir(root + QStringLiteral("/org/kde/kirigami/controls/qmldir"));
    QVERIFY(controlsQmldir.open(QFile::ReadOnly));
    QVERIFY(!controlsQmldir.readAll().contains("prefer "));

    QFile stamp(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                + QStringLiteral("/latte-dock-ng/kns-compat.stamp"));
    QVERIFY(stamp.open(QFile::ReadOnly));
    QCOMPARE(stamp.readAll().trimmed(), QByteArray("9"));
}

void KnsCompatUnitTest::optOutDoesNotWriteOverrides()
{
    QTemporaryDir home;
    QTemporaryDir data;
    QTemporaryDir systemQml;
    isolateHomeAndData(home, data);
    QVERIFY(systemQml.isValid());
    createSystemQmlRoot(systemQml.path());
    qputenv("LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS", QFile::encodeName(systemQml.path()));
    qputenv("LATTE_DISABLE_KNS_COMPAT", "1");

    ensureKnsCompat();

    QVERIFY(!QFile::exists(qmlRoot() + QStringLiteral("/org/kde/kirigami/templates/qmldir")));
    QVERIFY(!QFile::exists(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                           + QStringLiteral("/latte-dock-ng/kns-compat.stamp")));
}

void KnsCompatUnitTest::usesConfiguredSystemQmlRoot()
{
    QTemporaryDir home;
    QTemporaryDir data;
    QTemporaryDir missingSystemQml;
    QTemporaryDir systemQml;
    isolateHomeAndData(home, data);
    QVERIFY(missingSystemQml.isValid());
    QVERIFY(systemQml.isValid());
    createSystemQmlRoot(systemQml.path());
    qputenv("LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS",
            QFile::encodeName(missingSystemQml.path() + QLatin1Char(':') + systemQml.path()));

    ensureKnsCompat();

    QFile linkedNewstuff(qmlRoot() + QStringLiteral("/org/kde/newstuff/Button.qml"));
    QVERIFY(QFileInfo(linkedNewstuff).isSymLink());
    QCOMPARE(QFileInfo(linkedNewstuff).symLinkTarget(),
             systemQml.path() + QStringLiteral("/org/kde/newstuff/Button.qml"));
}

void KnsCompatUnitTest::ignoresRelativeConfiguredSystemQmlRoot()
{
    QTemporaryDir home;
    QTemporaryDir data;
    QTemporaryDir systemQml;
    isolateHomeAndData(home, data);
    QVERIFY(systemQml.isValid());
    QDir(QStringLiteral("relative-system-qml")).removeRecursively();
    createSystemQmlRoot(QStringLiteral("relative-system-qml"));
    createSystemQmlRoot(systemQml.path());
    qputenv("LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS",
            QFile::encodeName(QStringLiteral("relative-system-qml") + QLatin1Char(':') + systemQml.path()));

    ensureKnsCompat();

    QFile linkedNewstuff(qmlRoot() + QStringLiteral("/org/kde/newstuff/Button.qml"));
    QVERIFY(QFileInfo(linkedNewstuff).isSymLink());
    QCOMPARE(QFileInfo(linkedNewstuff).symLinkTarget(),
             systemQml.path() + QStringLiteral("/org/kde/newstuff/Button.qml"));

    QDir(QStringLiteral("relative-system-qml")).removeRecursively();
}

void KnsCompatUnitTest::usesConfiguredUserQmlRoot()
{
    QTemporaryDir home;
    QTemporaryDir data;
    QTemporaryDir systemQml;
    QTemporaryDir userQml;
    isolateHomeAndData(home, data);
    QVERIFY(systemQml.isValid());
    QVERIFY(userQml.isValid());
    createSystemQmlRoot(systemQml.path());
    qputenv("LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS", QFile::encodeName(systemQml.path()));
    qputenv("LATTE_KNS_COMPAT_USER_QML_ROOT", QFile::encodeName(userQml.path()));

    ensureKnsCompat();

    QVERIFY(QFile::exists(userQml.path() + QStringLiteral("/org/kde/kirigami/templates/qmldir")));
    QVERIFY(QFile::exists(userQml.path() + QStringLiteral("/org/kde/newstuff/qmldir")));
    QVERIFY(QFile::exists(userQml.path() + QStringLiteral("/org/kde/kirigami/controls/qmldir")));
    QVERIFY(!QFile::exists(qmlRoot() + QStringLiteral("/org/kde/kirigami/templates/qmldir")));
}

void KnsCompatUnitTest::ignoresRelativeConfiguredUserQmlRoot()
{
    QTemporaryDir home;
    QTemporaryDir data;
    QTemporaryDir systemQml;
    isolateHomeAndData(home, data);
    QVERIFY(systemQml.isValid());
    createSystemQmlRoot(systemQml.path());
    qputenv("LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS", QFile::encodeName(systemQml.path()));
    qputenv("LATTE_KNS_COMPAT_USER_QML_ROOT", "relative-qml-root");
    QDir(QStringLiteral("relative-qml-root")).removeRecursively();

    ensureKnsCompat();

    QVERIFY(QFile::exists(qmlRoot() + QStringLiteral("/org/kde/kirigami/templates/qmldir")));
    QVERIFY(!QFile::exists(QStringLiteral("relative-qml-root/org/kde/kirigami/templates/qmldir")));
}

void KnsCompatUnitTest::replacesSymlinkedPatchedFilesWithoutTouchingTarget()
{
    QTemporaryDir home;
    QTemporaryDir data;
    QTemporaryDir systemQml;
    QTemporaryDir userQml;
    isolateHomeAndData(home, data);
    QVERIFY(systemQml.isValid());
    QVERIFY(userQml.isValid());
    createSystemQmlRoot(systemQml.path());
    qputenv("LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS", QFile::encodeName(systemQml.path()));
    qputenv("LATTE_KNS_COMPAT_USER_QML_ROOT", QFile::encodeName(userQml.path()));

    const QString externalTargetPath = data.path() + QStringLiteral("/external-target.qml");
    QFile externalTarget(externalTargetPath);
    QVERIFY(externalTarget.open(QFile::WriteOnly | QFile::Truncate));
    externalTarget.write("external target must stay unchanged\n");
    externalTarget.close();

    const QString drawerPath = userQml.path() + QStringLiteral("/org/kde/kirigami/templates/private/DrawerHandle.qml");
    QVERIFY(QDir().mkpath(QFileInfo(drawerPath).absolutePath()));
    QVERIFY(QFile::link(externalTargetPath, drawerPath));

    ensureKnsCompat();

    QFile drawer(drawerPath);
    QVERIFY(drawer.open(QFile::ReadOnly));
    QVERIFY(drawer.readAll().contains("onActiveTranslationChanged"));
    QVERIFY(!QFileInfo(drawerPath).isSymLink());

    QFile externalTargetAfter(externalTargetPath);
    QVERIFY(externalTargetAfter.open(QFile::ReadOnly));
    QCOMPARE(externalTargetAfter.readAll(), QByteArray("external target must stay unchanged\n"));
}

void KnsCompatUnitTest::missingSystemQmlRootDoesNotCreatePartialOverrides()
{
    QTemporaryDir home;
    QTemporaryDir data;
    QTemporaryDir systemQml;
    isolateHomeAndData(home, data);
    QVERIFY(systemQml.isValid());
    qputenv("LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS", QFile::encodeName(systemQml.path()));

    ensureKnsCompat();

    QVERIFY(!QFile::exists(qmlRoot() + QStringLiteral("/org/kde/kirigami/templates/qmldir")));
    QVERIFY(!QFile::exists(qmlRoot() + QStringLiteral("/org/kde/newstuff/qmldir")));
    QVERIFY(!QFile::exists(qmlRoot() + QStringLiteral("/org/kde/kirigami/controls/qmldir")));
}

void KnsCompatUnitTest::systemTemplatesQmldirMirrorsNewTypes()
{
    QTemporaryDir home;
    QTemporaryDir data;
    QTemporaryDir systemQml;
    isolateHomeAndData(home, data);
    QVERIFY(systemQml.isValid());
    createSystemQmlRoot(systemQml.path());
    qputenv("LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS", QFile::encodeName(systemQml.path()));

    //! A system templates qmldir that declares a type added after the
    //! original hardcoded list (e.g. Badge, used by kicker/kickoff via
    //! controls/Badge.qml -> KT.Badge). The compat layer must mirror the
    //! system type list, otherwise widgets using Kirigami.Badge fail to
    //! load inside latte (empty dock slots).
    const QString sysTemplatesQmldir = systemQml.path() + QStringLiteral("/org/kde/kirigami/templates/qmldir");
    QVERIFY(QDir().mkpath(QFileInfo(sysTemplatesQmldir).absolutePath()));
    QFile qmldir(sysTemplatesQmldir);
    QVERIFY(qmldir.open(QIODevice::WriteOnly | QIODevice::Truncate));
    qmldir.write(QByteArrayLiteral("module org.kde.kirigami.templates\n"
                                   "typeinfo KirigamiTemplates.qmltypes\n"
                                   "prefer :/qt/qml/org/kde/kirigami/templates/\n"
                                   "AbstractCard 2.0 AbstractCard.qml\n"
                                   "Badge 2.0 Badge.qml\n"
                                   "Heading 2.0 Heading.qml\n"));
    qmldir.close();

    ensureKnsCompat();

    const QString compatQmldir = qmlRoot() + QStringLiteral("/org/kde/kirigami/templates/qmldir");
    QFile compat(compatQmldir);
    QVERIFY(compat.open(QIODevice::ReadOnly));
    const QByteArray compatText = compat.readAll();
    QVERIFY(compatText.contains("Badge 2.0 Badge.qml"));
    QVERIFY(compatText.contains("AbstractCard 2.0 AbstractCard.qml"));
    QVERIFY(!compatText.contains("prefer "));
}

void
KnsCompatUnitTest::systemTemplatesQmldirDropsPluginDirectives()
{
    QTemporaryDir home;
    QTemporaryDir data;
    QTemporaryDir systemQml;
    isolateHomeAndData(home, data);
    QVERIFY(systemQml.isValid());
    createSystemQmlRoot(systemQml.path());
    qputenv("LATTE_KNS_COMPAT_SYSTEM_QML_ROOTS", QFile::encodeName(systemQml.path()));

    //! The system templates qmldir names a C++ plugin (linktarget, optional plugin
    //! and classname) that the override directory does not have, so those lines
    //! must not reach the override. prefer is dropped too (see
    //! systemTemplatesQmldirMirrorsNewTypes). The module, typeinfo and type lines
    //! stay, or the types would stop resolving. The directive lines match the
    //! Kirigami 6 templates qmldir.
    const QString sysTemplatesQmldir = systemQml.path() + QStringLiteral("/org/kde/kirigami/templates/qmldir");
    QVERIFY(QDir().mkpath(QFileInfo(sysTemplatesQmldir).absolutePath()));
    QFile qmldir(sysTemplatesQmldir);
    QVERIFY(qmldir.open(QIODevice::WriteOnly | QIODevice::Truncate));
    qmldir.write(QByteArrayLiteral("module org.kde.kirigami.templates\n"
                                   "linktarget KF6KirigamiTemplates\n"
                                   "optional plugin KirigamiTemplatesplugin\n"
                                   "classname org_kde_kirigami_templatesPlugin\n"
                                   "typeinfo KirigamiTemplates.qmltypes\n"
                                   "prefer :/qt/qml/org/kde/kirigami/templates/\n"
                                   "Badge 2.0 Badge.qml\n"
                                   "Heading 2.0 Heading.qml\n"));
    qmldir.close();

    ensureKnsCompat();

    const QString compatQmldir = qmlRoot() + QStringLiteral("/org/kde/kirigami/templates/qmldir");
    QFile compat(compatQmldir);
    QVERIFY(compat.open(QIODevice::ReadOnly));
    QCOMPARE(compat.readAll(),
             QByteArrayLiteral("module org.kde.kirigami.templates\n"
                               "typeinfo KirigamiTemplates.qmltypes\n"
                               "Badge 2.0 Badge.qml\n"
                               "Heading 2.0 Heading.qml\n"));
}

QTEST_GUILESS_MAIN(KnsCompatUnitTest)

#include "knscompatunittest.moc"
