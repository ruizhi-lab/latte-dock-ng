/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef VIEWTEMPLATENAMES_H
#define VIEWTEMPLATENAMES_H

// Qt
#include <QList>
#include <QString>

namespace Latte {
namespace Templates {

//! A view template as Templates::Manager lists it: its file and the name the UI shows.
struct ViewTemplateEntry
{
    QString id;   //! path of the *.view.latte file
    QString name; //! shown name: translated for system templates, the base name for user templates
};

//! Untranslated name of a view template: the file name without the ".view.latte" suffix.
//! This is the name that --available-dock-templates prints. Dots inside the name are kept,
//! so "My.Dock.view.latte" gives "My.Dock"; QFileInfo::baseName() would cut it at the first dot.
QString viewTemplateBaseName(const QString &filePath);

//! Returns the id of the template that a requested name selects, or an empty string.
//!
//! Trigger: --add-dock and the D-Bus addView() accept a name. In a translated session the
//! listed name ("Default Dock") differs from the shown name ("默认停靠栏"), and the lookup
//! used to match only the shown name, so the listed name added nothing and logged nothing.
//! Authoritative source: the template files. Both names come from the same entry, and the
//! shown name is derived from the file. Shown names are tried first, in table order, which
//! keeps every name that selected a template before selecting the same one. Base names are
//! the fallback. The caller must use the same table for the start-up check and the running
//! dock, or one of them would accept a name the other rejects.
QString selectViewTemplateId(const QList<ViewTemplateEntry> &templates, const QString &name);

}
}

#endif
