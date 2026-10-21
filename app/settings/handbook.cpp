/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "handbook.h"

#include <utility>

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KMessageBox>

namespace Latte::Settings {
QUrl
handbookUrl(const QStringList &languages, const QString &section, const QStringList &dataRoots)
{
    QStringList candidates;
    for (const QString &language : languages) {
        const QLocale locale(language);
        QStringList localized;
        // Preserve the raw KDE language code (for example ca@valencia or
        // sr@latin) because QLocale may normalize it to a different spelling.
        // A guide installed under the raw code must win over the English fallback.
        if (!language.isEmpty()) {
            localized << language;
        }
        if (locale.language() == QLocale::Chinese) {
            localized << (locale.script() == QLocale::TraditionalHanScript ? QStringLiteral("zh_TW") : QStringLiteral("zh_CN"));
        } else if (locale.language() != QLocale::C) {
            localized << locale.name() << locale.bcp47Name().section(QLatin1Char('-'), 0, 0);
        }
        for (const QString &candidate : std::as_const(localized)) {
            if (!candidates.contains(candidate)) {
                candidates << candidate;
            }
        }
    }
    candidates << QStringLiteral("en");

    // Language priority precedes install prefix priority: a user installation
    // may lack the requested translation while the system installation has it.
    // KDE's UI language list is authoritative; English is the offline fallback.
    for (const QString &language : std::as_const(candidates)) {
        for (const QString &root : dataRoots) {
            const QString file = QDir(root).filePath(QStringLiteral("latte-dock-ng/help/%1/index.html").arg(language));
            if (QFileInfo(file).isFile()) {
                QUrl url = QUrl::fromLocalFile(file);
                url.setFragment(section);
                return url;
            }
        }
    }
    return {};
}

void
openHandbook(const QString &section)
{
    const QUrl url = handbookUrl(KLocalizedString::languages(), section, QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation));
    if (url.isEmpty()) {
        KMessageBox::error(nullptr, i18nc("@info", "The Latte Dock NG user guide is not installed. Reinstall Latte Dock NG to restore the help files."));
    } else if (!QDesktopServices::openUrl(url)) {
        KMessageBox::error(nullptr, i18nc("@info", "Could not open the user guide. Check that a default web browser is configured."));
    }
}
}
