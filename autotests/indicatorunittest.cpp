/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "factory.h"
#include "importer.h"

#include <KArchive/KZip>
#include <KPluginMetaData>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

namespace {
QStringList s_standardPaths;

QByteArray indicatorMetadata(const QString &pluginId,
                             const QString &category = QStringLiteral("Latte Indicator"),
                             const QString &mainScript = QStringLiteral("ui/main.qml"))
{
    return QStringLiteral(R"({
    "KPlugin": {
        "Category": "%1",
        "Id": "%2",
        "Name": "Test Indicator",
        "ServiceTypes": ["Latte/Indicator"]
    },
    "X-Latte-MainScript": "%3",
    "X-Plasma-API": "declarativeappletscript"
})").arg(category, pluginId, mainScript).toUtf8();
}

QString writeFile(const QString &path, const QByteArray &contents)
{
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return QString();
    }

    file.write(contents);
    return path;
}

QString createIndicatorArchive(const QString &archivePath, const QString &rootDirectory, const QString &pluginId)
{
    KZip archive(archivePath);
    if (!archive.open(QIODevice::WriteOnly)) {
        return QString();
    }

    archive.writeFile(rootDirectory + QStringLiteral("/metadata.json"), indicatorMetadata(pluginId));
    archive.writeFile(rootDirectory + QStringLiteral("/package/ui/main.qml"), QByteArray("import QtQml 2.15\nQtObject {}\n"));
    archive.close();
    return archivePath;
}
}

namespace Latte {
namespace Layouts {
QStringList Importer::standardPaths(bool localfirst)
{
    Q_UNUSED(localfirst)

    return s_standardPaths;
}
}
}

class IndicatorUnitTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void metadataFileAbsolutePathPrefersJson();
    void metadataValidationRequiresIndicatorCategoryAndMainScript();
    void importIndicatorFileInstallsZipIntoIsolatedDataPath();
    void importIndicatorFileRejectsInvalidArchive();
    void importIndicatorFileRejectsInvalidMetadata();
    void importIndicatorFileReportsUpdatedStateForExistingIndicator();
};

void IndicatorUnitTest::metadataFileAbsolutePathPrefersJson()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QCOMPARE(Latte::Indicator::Factory::metadataFileAbsolutePath(dir.path()), QString());

    const QString desktopPath = writeFile(dir.path() + QStringLiteral("/metadata.desktop"), QByteArray("[Desktop Entry]\nName=Legacy\n"));
    QVERIFY(!desktopPath.isEmpty());
    QCOMPARE(Latte::Indicator::Factory::metadataFileAbsolutePath(dir.path()), desktopPath);

    const QString jsonPath = writeFile(dir.path() + QStringLiteral("/metadata.json"), indicatorMetadata(QStringLiteral("org.kde.latte.test")));
    QVERIFY(!jsonPath.isEmpty());
    QCOMPARE(Latte::Indicator::Factory::metadataFileAbsolutePath(dir.path()), jsonPath);
}

void IndicatorUnitTest::metadataValidationRequiresIndicatorCategoryAndMainScript()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QString validPath = writeFile(dir.path() + QStringLiteral("/valid.json"), indicatorMetadata(QStringLiteral("org.kde.latte.valid")));
    KPluginMetaData validMetadata = KPluginMetaData::fromJsonFile(validPath);
    QVERIFY(Latte::Indicator::Factory::metadataAreValid(validMetadata));

    QString wrongCategoryPath = writeFile(dir.path() + QStringLiteral("/wrongcategory.json"),
                                          indicatorMetadata(QStringLiteral("org.kde.latte.wrongcategory"), QStringLiteral("Other")));
    KPluginMetaData wrongCategory = KPluginMetaData::fromJsonFile(wrongCategoryPath);
    QVERIFY(!Latte::Indicator::Factory::metadataAreValid(wrongCategory));

    QString missingScriptPath = writeFile(dir.path() + QStringLiteral("/missingscript.json"),
                                          indicatorMetadata(QStringLiteral("org.kde.latte.missingscript"), QStringLiteral("Latte Indicator"), QString()));
    KPluginMetaData missingScript = KPluginMetaData::fromJsonFile(missingScriptPath);
    QVERIFY(!Latte::Indicator::Factory::metadataAreValid(missingScript));
}

void IndicatorUnitTest::importIndicatorFileInstallsZipIntoIsolatedDataPath()
{
    QTemporaryDir dataRoot;
    QVERIFY(dataRoot.isValid());
    QVERIFY(QDir(dataRoot.path()).mkpath(QStringLiteral("latte/indicators")));

    s_standardPaths = {dataRoot.path()};

    QTemporaryDir archiveRoot;
    QVERIFY(archiveRoot.isValid());

    const QString pluginId = QStringLiteral("org.kde.latte.testimport");
    const QString archivePath = createIndicatorArchive(archiveRoot.path() + QStringLiteral("/indicator.zip"),
                                                       QStringLiteral("testimport"),
                                                       pluginId);
    QVERIFY(!archivePath.isEmpty());

    QCOMPARE(static_cast<int>(Latte::Indicator::Factory::importIndicatorFile(archivePath)),
             static_cast<int>(Latte::ImportExport::InstalledState));

    const QString installPath = dataRoot.path() + QStringLiteral("/latte/indicators/") + pluginId;
    QVERIFY(QFileInfo::exists(installPath + QStringLiteral("/metadata.json")));
    QVERIFY(QFileInfo::exists(installPath + QStringLiteral("/package/ui/main.qml")));
}

void IndicatorUnitTest::importIndicatorFileRejectsInvalidArchive()
{
    QTemporaryDir dataRoot;
    QVERIFY(dataRoot.isValid());
    s_standardPaths = {dataRoot.path()};

    QTemporaryDir archiveRoot;
    QVERIFY(archiveRoot.isValid());

    const QString invalidArchive = writeFile(archiveRoot.path() + QStringLiteral("/indicator.zip"), QByteArray("not an archive"));
    QVERIFY(!invalidArchive.isEmpty());

    QCOMPARE(static_cast<int>(Latte::Indicator::Factory::importIndicatorFile(invalidArchive)),
             static_cast<int>(Latte::ImportExport::FailedState));
}

void IndicatorUnitTest::importIndicatorFileRejectsInvalidMetadata()
{
    QTemporaryDir dataRoot;
    QVERIFY(dataRoot.isValid());
    QVERIFY(QDir(dataRoot.path()).mkpath(QStringLiteral("latte/indicators")));
    s_standardPaths = {dataRoot.path()};

    QTemporaryDir archiveRoot;
    QVERIFY(archiveRoot.isValid());

    const QString pluginId = QStringLiteral("org.kde.latte.invalidimport");
    const QString archivePath = archiveRoot.path() + QStringLiteral("/invalid-indicator.zip");

    KZip archive(archivePath);
    QVERIFY(archive.open(QIODevice::WriteOnly));
    archive.writeFile(QStringLiteral("invalid/metadata.json"), indicatorMetadata(pluginId, QStringLiteral("Other")));
    archive.writeFile(QStringLiteral("invalid/package/ui/main.qml"), QByteArray("import QtQml 2.15\nQtObject {}\n"));
    archive.close();

    QCOMPARE(static_cast<int>(Latte::Indicator::Factory::importIndicatorFile(archivePath)),
             static_cast<int>(Latte::ImportExport::FailedState));
    QVERIFY(!QFileInfo::exists(dataRoot.path() + QStringLiteral("/latte/indicators/") + pluginId));
}

void IndicatorUnitTest::importIndicatorFileReportsUpdatedStateForExistingIndicator()
{
    QTemporaryDir dataRoot;
    QVERIFY(dataRoot.isValid());
    QVERIFY(QDir(dataRoot.path()).mkpath(QStringLiteral("latte/indicators")));

    s_standardPaths = {dataRoot.path()};

    const QString pluginId = QStringLiteral("org.kde.latte.updateimport");
    const QString installPath = dataRoot.path() + QStringLiteral("/latte/indicators/") + pluginId;
    QVERIFY(QDir().mkpath(installPath));
    QVERIFY(!writeFile(installPath + QStringLiteral("/stale.txt"), QByteArray("old")).isEmpty());

    QTemporaryDir archiveRoot;
    QVERIFY(archiveRoot.isValid());

    const QString archivePath = createIndicatorArchive(archiveRoot.path() + QStringLiteral("/indicator.zip"),
                                                       QStringLiteral("updateimport"),
                                                       pluginId);
    QVERIFY(!archivePath.isEmpty());

    QCOMPARE(static_cast<int>(Latte::Indicator::Factory::importIndicatorFile(archivePath)),
             static_cast<int>(Latte::ImportExport::UpdatedState));
    QVERIFY(!QFileInfo::exists(installPath + QStringLiteral("/stale.txt")));
    QVERIFY(QFileInfo::exists(installPath + QStringLiteral("/package/ui/main.qml")));
}

QTEST_MAIN(IndicatorUnitTest)

#include "indicatorunittest.moc"
