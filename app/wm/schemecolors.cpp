/*
    SPDX-FileCopyrightText: 2018 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later

*/

#include "schemecolors.h"

// local
#include <config-latte.h>
#include "../layouts/importer.h"
#include "../tools/commontools.h"

// Qt
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLatin1String>

// KDE
#include <KConfigGroup>
#include <KDirWatch>
#include <KSharedConfig>

namespace Latte {
namespace WindowSystem {

SchemeColors::SchemeColors(QObject *parent, QString scheme, bool plasmaTheme) :
    QObject(parent),
    m_basedOnPlasmaTheme(plasmaTheme)
{
    QString pSchemeFile = possibleSchemeFile(scheme);

    if (QFileInfo::exists(pSchemeFile)) {
        setSchemeFile(pSchemeFile);
        m_schemeName = schemeName(pSchemeFile);

        //! track scheme file for changes
        KDirWatch::self()->addFile(m_schemeFile);

        connect(KDirWatch::self(), &KDirWatch::created, this, [ & ](const QString & path) {
            if (path == m_schemeFile) {
                updateScheme();
            }
        });

        connect(KDirWatch::self(), &KDirWatch::dirty, this, [ & ](const QString & path) {
            if (path == m_schemeFile) {
                updateScheme();
            }
        });
    }

    updateScheme();
}

SchemeColors::~SchemeColors()
{
    ///
}

QColor SchemeColors::backgroundColor() const
{
    return m_activeBackgroundColor;
}

QColor SchemeColors::textColor() const
{
    return m_activeTextColor;
}

QColor SchemeColors::inactiveBackgroundColor() const
{
    return m_inactiveBackgroundColor;
}

QColor SchemeColors::inactiveTextColor() const
{
    return m_inactiveTextColor;
}

QColor SchemeColors::highlightColor() const
{
    return m_highlightColor;
}

QColor SchemeColors::highlightedTextColor() const
{
    return m_highlightedTextColor;
}

QColor SchemeColors::positiveTextColor() const
{
    return m_positiveTextColor;
}

QColor SchemeColors::neutralTextColor() const
{
    return m_neutralTextColor;
}

QColor SchemeColors::negativeTextColor() const
{
    return m_negativeTextColor;
}

QColor SchemeColors::buttonTextColor() const
{
    return m_buttonTextColor;
}

QColor SchemeColors::buttonBackgroundColor() const
{
    return m_buttonBackgroundColor;
}

QColor SchemeColors::buttonHoverColor() const
{
    return m_buttonHoverColor;
}

QColor SchemeColors::buttonFocusColor() const
{
    return m_buttonFocusColor;
}

QString SchemeColors::schemeName() const
{
    return m_schemeName;
}

QString SchemeColors::SchemeColors::schemeFile() const
{
    return m_schemeFile;
}

void SchemeColors::setSchemeFile(QString file)
{
    if (m_schemeFile == file) {
        return;
    }

    m_schemeFile = file;
    Q_EMIT schemeFileChanged();
}

QString SchemeColors::possibleSchemeFile(QString scheme)
{
    if (scheme == QLatin1String("kdeglobals")
        || (scheme.endsWith(QStringLiteral("kdeglobals")) && QFileInfo::exists(scheme))) {
        // do nothing, accept kdeglobals case
    } else if (scheme.startsWith(QLatin1Char('/')) && scheme.endsWith(QStringLiteral("colors")) && QFileInfo::exists(scheme)) {
        return scheme;
    }

    QString schemePath;
    QString tempScheme = scheme;

    if (scheme == QLatin1String("kdeglobals")
        || (scheme.endsWith(QStringLiteral("kdeglobals")) && QFileInfo::exists(scheme))) {
        QString settingsFile = Latte::configPath() + QLatin1String("/kdeglobals");

        bool supportsAutoAccentColor{false}; // introduced on plasma 5.25

        if (QFileInfo::exists(settingsFile)) {
            KSharedConfigPtr filePtr = KSharedConfig::openConfig(settingsFile);
            KConfigGroup wmGroup = KConfigGroup(filePtr, QStringLiteral("WM"));
            KConfigGroup generalGroup = KConfigGroup(filePtr, QStringLiteral("General"));

            if (wmGroup.hasKey(QStringLiteral("activeBackground"))) {
                supportsAutoAccentColor = true;
            } else {
                tempScheme = generalGroup.readEntry(QStringLiteral("ColorScheme"), "BreezeLight");
            }
        }

        if (supportsAutoAccentColor) {
            schemePath = Latte::configPath() + QLatin1String("/kdeglobals");
        } else {
            schemePath = Layouts::Importer::standardPath(QStringLiteral("color-schemes/") + tempScheme + QStringLiteral(".colors"));
        }
    } else {
        schemePath = Layouts::Importer::standardPath(QStringLiteral("color-schemes/") + tempScheme + QStringLiteral(".colors"));
    }

    if (schemePath.isEmpty() || !QFileInfo::exists(schemePath)) {
        //! remove all whitespaces and "-" from scheme in order to access correctly its file
        QString schemeNameSimplified = tempScheme.simplified().remove(QLatin1Char(' ')).remove(QLatin1Char('-'));

        schemePath = Layouts::Importer::standardPath(QStringLiteral("color-schemes/") + schemeNameSimplified + QStringLiteral(".colors"));
    }

    if (QFileInfo::exists(schemePath)) {
        return schemePath;
    }

    return QString();
}

QString SchemeColors::schemeName(QString originalFile)
{
    if (originalFile.endsWith(QStringLiteral("kdeglobals")) && QFileInfo::exists(originalFile)) {
        return QStringLiteral("kdeglobals");
    }

    if (!(originalFile.startsWith(QLatin1Char('/')) && originalFile.endsWith(QStringLiteral("colors")) && QFileInfo::exists(originalFile))) {
        return QString();
    }

    QString fileNameNoExt =  originalFile;

    int lastSlash = originalFile.lastIndexOf(QLatin1Char('/'));

    if (lastSlash >= 0) {
        fileNameNoExt.remove(0, lastSlash + 1);
    }

    if (fileNameNoExt.endsWith(QStringLiteral(".colors"))) {
        fileNameNoExt.remove(QStringLiteral(".colors"));
    }

    KSharedConfigPtr filePtr = KSharedConfig::openConfig(originalFile);
    KConfigGroup generalGroup = KConfigGroup(filePtr, QStringLiteral("General"));

    return generalGroup.readEntry(QStringLiteral("Name"), fileNameNoExt);
}

void SchemeColors::updateScheme()
{
    if (m_schemeFile.isEmpty() || !QFileInfo::exists(m_schemeFile)) {
        return;
    }

    KSharedConfigPtr filePtr = KSharedConfig::openConfig(m_schemeFile);
    KConfigGroup wmGroup = KConfigGroup(filePtr, QStringLiteral("WM"));
    KConfigGroup selGroup = KConfigGroup(filePtr, QStringLiteral("Colors:Selection"));
    //KConfigGroup viewGroup = KConfigGroup(filePtr, QStringLiteral("Colors:View"));
    KConfigGroup windowGroup = KConfigGroup(filePtr, QStringLiteral("Colors:Window"));
    KConfigGroup buttonGroup = KConfigGroup(filePtr, QStringLiteral("Colors:Button"));

    if (!m_basedOnPlasmaTheme) {
        m_activeBackgroundColor = wmGroup.readEntry(QStringLiteral("activeBackground"), QColor());
        m_activeTextColor = wmGroup.readEntry(QStringLiteral("activeForeground"), QColor());
        m_inactiveBackgroundColor = wmGroup.readEntry(QStringLiteral("inactiveBackground"), QColor());
        m_inactiveTextColor = wmGroup.readEntry(QStringLiteral("inactiveForeground"), QColor());
    } else {
        m_activeBackgroundColor = windowGroup.readEntry(QStringLiteral("BackgroundNormal"), QColor());
        m_activeTextColor = windowGroup.readEntry(QStringLiteral("ForegroundNormal"), QColor());
        m_inactiveBackgroundColor = windowGroup.readEntry(QStringLiteral("BackgroundAlternate"), QColor());
        m_inactiveTextColor = windowGroup.readEntry(QStringLiteral("ForegroundInactive"), QColor());
    }

    m_highlightColor = selGroup.readEntry(QStringLiteral("BackgroundNormal"), QColor());
    m_highlightedTextColor = selGroup.readEntry(QStringLiteral("ForegroundNormal"), QColor());

    m_positiveTextColor = windowGroup.readEntry(QStringLiteral("ForegroundPositive"), QColor());
    m_neutralTextColor = windowGroup.readEntry(QStringLiteral("ForegroundNeutral"), QColor());;
    m_negativeTextColor = windowGroup.readEntry(QStringLiteral("ForegroundNegative"), QColor());

    m_buttonTextColor = buttonGroup.readEntry(QStringLiteral("ForegroundNormal"), QColor());
    m_buttonBackgroundColor = buttonGroup.readEntry(QStringLiteral("BackgroundNormal"), QColor());
    m_buttonHoverColor = buttonGroup.readEntry(QStringLiteral("DecorationHover"), QColor());
    m_buttonFocusColor = buttonGroup.readEntry(QStringLiteral("DecorationFocus"), QColor());

    Q_EMIT colorsChanged();
}

}
}
