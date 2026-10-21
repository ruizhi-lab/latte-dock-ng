/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "../app/settings/handbook.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class HandbookTest : public QObject
{
    Q_OBJECT
  private Q_SLOTS:
    void languageSelection_data();
    void languageSelection();
    void missingTranslationFallsBack();
    void additionalLanguageCanBeInstalled();
    void rawLanguageCodeIsPreserved();
    void missingInstallationIsReported();
};

static bool
installGuide(const QString &root, const QString &language)
{
    const QString directory = root + QStringLiteral("/latte-dock-ng/help/") + language;
    if (!QDir().mkpath(directory)) {
        return false;
    }
    QFile guide(directory + QStringLiteral("/index.html"));
    return guide.open(QIODevice::WriteOnly) && guide.write("<!doctype html>") > 0;
}

void
HandbookTest::languageSelection_data()
{
    QTest::addColumn<QStringList>("languages");
    QTest::addColumn<QString>("expected");
    QTest::newRow("english") << QStringList{ QStringLiteral("en_GB") } << QStringLiteral("en");
    QTest::newRow("simplified") << QStringList{ QStringLiteral("zh_CN") } << QStringLiteral("zh_CN");
    QTest::newRow("simplified-script") << QStringList{ QStringLiteral("zh-Hans") } << QStringLiteral("zh_CN");
    QTest::newRow("traditional") << QStringList{ QStringLiteral("zh_TW") } << QStringLiteral("zh_TW");
    QTest::newRow("hong-kong") << QStringList{ QStringLiteral("zh_HK") } << QStringLiteral("zh_TW");
    QTest::newRow("traditional-script") << QStringList{ QStringLiteral("zh-Hant") } << QStringLiteral("zh_TW");
    QTest::newRow("unsupported") << QStringList{ QStringLiteral("de_DE") } << QStringLiteral("en");
    QTest::newRow("secondary-language") << QStringList{ QStringLiteral("de_DE"), QStringLiteral("zh_CN") } << QStringLiteral("zh_CN");
    QTest::newRow("english-first") << QStringList{ QStringLiteral("en_US"), QStringLiteral("zh_CN") } << QStringLiteral("en");
}

void
HandbookTest::languageSelection()
{
    QFETCH(QStringList, languages);
    QFETCH(QString, expected);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    for (const QString &language : { QStringLiteral("en"), QStringLiteral("zh_CN"), QStringLiteral("zh_TW") }) {
        QVERIFY(installGuide(directory.path(), language));
    }
    const QUrl url = Latte::Settings::handbookUrl(languages, QStringLiteral("edit-dock"), { directory.path() });
    QVERIFY(url.isLocalFile());
    QVERIFY(url.toLocalFile().endsWith(QStringLiteral("/%1/index.html").arg(expected)));
    QCOMPARE(url.fragment(), QStringLiteral("edit-dock"));
}

void
HandbookTest::missingTranslationFallsBack()
{
    QTemporaryDir user;
    QTemporaryDir system;
    QVERIFY(user.isValid());
    QVERIFY(system.isValid());
    QVERIFY(installGuide(user.path(), QStringLiteral("en")));
    const QStringList roots{ user.path(), system.path() };
    QUrl url = Latte::Settings::handbookUrl({ QStringLiteral("zh_CN") }, QStringLiteral("configure-latte"), roots);
    QVERIFY(url.toLocalFile().startsWith(user.path()));
    QVERIFY(url.toLocalFile().endsWith(QStringLiteral("/en/index.html")));
    QVERIFY(installGuide(system.path(), QStringLiteral("zh_CN")));
    url = Latte::Settings::handbookUrl({ QStringLiteral("zh_CN") }, QStringLiteral("copies-and-clones"), roots);
    QVERIFY(url.toLocalFile().startsWith(system.path()));
    QCOMPARE(url.fragment(), QStringLiteral("copies-and-clones"));
}

void
HandbookTest::additionalLanguageCanBeInstalled()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(installGuide(directory.path(), QStringLiteral("en")));
    QVERIFY(installGuide(directory.path(), QStringLiteral("fr")));
    const QUrl url = Latte::Settings::handbookUrl({ QStringLiteral("fr_CA") }, QStringLiteral("edit-dock"), { directory.path() });
    QVERIFY(url.toLocalFile().endsWith(QStringLiteral("/fr/index.html")));
}

void
HandbookTest::rawLanguageCodeIsPreserved()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(installGuide(directory.path(), QStringLiteral("en")));
    QVERIFY(installGuide(directory.path(), QStringLiteral("ca@valencia")));
    const QUrl url = Latte::Settings::handbookUrl({ QStringLiteral("ca@valencia") }, QStringLiteral("edit-dock"), { directory.path() });
    QVERIFY(url.toLocalFile().endsWith(QStringLiteral("/ca@valencia/index.html")));
    QCOMPARE(url.fragment(), QStringLiteral("edit-dock"));
}

void
HandbookTest::missingInstallationIsReported()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(Latte::Settings::handbookUrl({ QStringLiteral("en") }, QStringLiteral("edit-dock"), { directory.path() }).isEmpty());
}

QTEST_GUILESS_MAIN(HandbookTest)
#include "handbooktest.moc"
