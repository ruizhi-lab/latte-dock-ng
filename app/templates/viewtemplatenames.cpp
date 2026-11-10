/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "viewtemplatenames.h"

// Qt
#include <QFileInfo>

namespace Latte {
namespace Templates {

QString viewTemplateBaseName(const QString &filePath)
{
    //! Only the ".view.latte" suffix is removed. QFileInfo::baseName() stops at the first dot, which
    //! gave a shown name "My" for "My.Dock.view.latte" while --available-dock-templates listed "My.Dock".
    const QLatin1String suffix(".view.latte");

    QString name = QFileInfo(filePath).fileName();

    if (name.endsWith(suffix)) {
        name.chop(suffix.size());
    }

    return name;
}

QString selectViewTemplateId(const QList<ViewTemplateEntry> &templates, const QString &name)
{
    if (name.isEmpty()) {
        return QString();
    }

    //! Pass 1 is the rule the template manager always had: the first entry whose shown name matches.
    //! Pass 2 is the fallback for the untranslated base name. Do not swap the passes: a base name that
    //! equals another template's shown name would then select a different template than the UI shows.
    for (const ViewTemplateEntry &entry : templates) {
        if (entry.name == name) {
            return entry.id;
        }
    }

    for (const ViewTemplateEntry &entry : templates) {
        if (viewTemplateBaseName(entry.id) == name) {
            return entry.id;
        }
    }

    return QString();
}

}
}
