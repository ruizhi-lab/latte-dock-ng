/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef LATTE_HANDBOOK_H
#define LATTE_HANDBOOK_H

#include <QStringList>
#include <QUrl>

namespace Latte::Settings {
QUrl
handbookUrl(const QStringList &languages, const QString &section, const QStringList &dataRoots);
void
openHandbook(const QString &section);
}
#endif
