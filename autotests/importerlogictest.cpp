/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KConfigGroup>
#include <KSharedConfig>

#include <QTemporaryDir>
#include <QTest>

// Replicates static helper logic from app/layouts/importer.cpp

namespace ImporterLogic {

enum FileVersion { Unknown = -1, LayoutV2 = 2, ConfigV2 = 3 };

QString nameOfConfigFile(const QString &fileName)
{
    int lastSlash = fileName.lastIndexOf(QLatin1Char('/'));
    QString temp = fileName;
    QString layoutName = temp.remove(0, lastSlash + 1);
    int ext = layoutName.lastIndexOf(QStringLiteral(".latterc"));
    return (ext >= 0) ? layoutName.remove(ext, 8) : layoutName;
}

FileVersion fileVersion(const QString &file, KSharedConfig::Ptr config = {})
{
    auto cfg = config ? config : KSharedConfig::openConfig(file);
    if (!QFile::exists(file))
        return Unknown;

    if (file.endsWith(QStringLiteral(".layout.latte"))) {
        KConfigGroup lg(cfg, QStringLiteral("LayoutSettings"));
        int v = lg.readEntry("version", 1);
        return (v == 2) ? LayoutV2 : Unknown;
    }
    if (file.endsWith(QStringLiteral(".latterc"))) {
        KConfigGroup lg(cfg, QStringLiteral("LayoutSettings"));
        int v = lg.readEntry("version", 1);
        return (v == 2) ? ConfigV2 : Unknown;
    }
    return Unknown;
}

QString layoutUserFilePath(const QString &layoutName)
{
    return layoutName.isEmpty() ? QString()
        : QStringLiteral("/tmp/latte-test/latte/") + layoutName + QStringLiteral(".layout.latte");
}

} // namespace ImporterLogic

class ImporterLogicTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void nameStripsPathAndExtension();
    void nameWithNoExtensionPreserved();
    void nameFromEmptyString();

    void unknownFileTypeForNonexistent();
    void layoutV2Recognized();
    void layoutV1IsUnknown();

    void userFilePathEndsWithExtension();
    void emptyNameProducesEmptyPath();
};

void ImporterLogicTest::nameStripsPathAndExtension()
{
    QCOMPARE(ImporterLogic::nameOfConfigFile(QStringLiteral("/home/user/myconfig.latterc")),
             QStringLiteral("myconfig"));
    QCOMPARE(ImporterLogic::nameOfConfigFile(QStringLiteral("dock.latterc")),
             QStringLiteral("dock"));
}

void ImporterLogicTest::nameWithNoExtensionPreserved()
{
    QCOMPARE(ImporterLogic::nameOfConfigFile(QStringLiteral("/path/justname")),
             QStringLiteral("justname"));
}

void ImporterLogicTest::nameFromEmptyString()
{
    QCOMPARE(ImporterLogic::nameOfConfigFile(QString()), QString());
}

void ImporterLogicTest::unknownFileTypeForNonexistent()
{
    QCOMPARE(ImporterLogic::fileVersion(QStringLiteral("/nonexistent.layout.latte")),
             ImporterLogic::Unknown);
}

void ImporterLogicTest::layoutV2Recognized()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/v2.layout.latte");
    auto cfg = KSharedConfig::openConfig(path);
    KConfigGroup(cfg, QStringLiteral("LayoutSettings")).writeEntry("version", 2);
    cfg->sync();
    QCOMPARE(ImporterLogic::fileVersion(path, cfg), ImporterLogic::LayoutV2);
}

void ImporterLogicTest::layoutV1IsUnknown()
{
    QTemporaryDir dir;
    const QString path = dir.path() + QStringLiteral("/v1.layout.latte");
    auto cfg = KSharedConfig::openConfig(path);
    KConfigGroup(cfg, QStringLiteral("LayoutSettings")).writeEntry("version", 1);
    cfg->sync();
    QCOMPARE(ImporterLogic::fileVersion(path, cfg), ImporterLogic::Unknown);
}

void ImporterLogicTest::userFilePathEndsWithExtension()
{
    const QString path = ImporterLogic::layoutUserFilePath(QStringLiteral("MyDock"));
    QVERIFY(path.endsWith(QStringLiteral("/MyDock.layout.latte")));
}

void ImporterLogicTest::emptyNameProducesEmptyPath()
{
    QVERIFY(ImporterLogic::layoutUserFilePath(QString()).isEmpty());
}

QTEST_MAIN(ImporterLogicTest)
#include "importerlogictest.moc"
