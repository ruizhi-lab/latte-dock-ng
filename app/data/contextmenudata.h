/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CONTEXTMENUDATA_H
#define CONTEXTMENUDATA_H

// Qt
#include <QStringList>

namespace Latte {
namespace Data {
namespace ContextMenu {

//! The action ids are accessors, not namespace-scope objects. A namespace-scope QString
//! or QStringList is built during static initialization, so a global initializer in
//! another translation unit could read it while it is still empty. Each accessor builds
//! its value on first use; clazy reports the namespace-scope form as non-pod-global-static.
inline const QString &ADDVIEWACTION()
{
    static const QString value = QStringLiteral("_add_view");
    return value;
}

inline const QString &ADDWIDGETSACTION()
{
    static const QString value = QStringLiteral("_add_latte_widgets");
    return value;
}

inline const QString &DUPLICATEVIEWACTION()
{
    static const QString value = QStringLiteral("_duplicate_view"); /*used inside add view submenu*/
    return value;
}

inline const QString &EDITVIEWACTION()
{
    static const QString value = QStringLiteral("_edit_view");
    return value;
}

inline const QString &EXPORTVIEWTEMPLATEACTION()
{
    static const QString value = QStringLiteral("_export_view");
    return value;
}

inline const QString &LAYOUTSACTION()
{
    static const QString value = QStringLiteral("_layouts");
    return value;
}

inline const QString &MOVEVIEWACTION()
{
    static const QString value = QStringLiteral("_move_view");
    return value;
}

inline const QString &PRINTACTION()
{
    static const QString value = QStringLiteral("_print");
    return value;
}

inline const QString &PREFERENCESACTION()
{
    static const QString value = QStringLiteral("_preferences");
    return value;
}

inline const QString &REMOVEVIEWACTION()
{
    static const QString value = QStringLiteral("_remove_view");
    return value;
}

inline const QString &QUITLATTEACTION()
{
    static const QString value = QStringLiteral("_quit_latte");
    return value;
}

inline const QString &SECTIONACTION()
{
    static const QString value = QStringLiteral("_latte_section");
    return value;
}

inline const QString &SEPARATOR1ACTION()
{
    static const QString value = QStringLiteral("_separator1");
    return value;
}

inline const QStringList &ACTIONSEDITORDER()
{
    static const QStringList value = {LAYOUTSACTION(),
                                      PREFERENCESACTION(),
                                      QUITLATTEACTION(),
                                      SEPARATOR1ACTION(),
                                      ADDWIDGETSACTION(),
                                      ADDVIEWACTION(),
                                      MOVEVIEWACTION(),
                                      EXPORTVIEWTEMPLATEACTION(),
                                      REMOVEVIEWACTION()
                                     };
    return value;
}

inline const QStringList &ACTIONSALWAYSVISIBLE()
{
    static const QStringList value = {LAYOUTSACTION(),
                                      PREFERENCESACTION(),
                                      QUITLATTEACTION(),
                                      SEPARATOR1ACTION(),
                                      ADDWIDGETSACTION(),
                                      ADDVIEWACTION()
                                     };
    return value;
}

inline const QStringList &ACTIONSALWAYSHIDDEN()
{
    static const QStringList value = {PRINTACTION()};
    return value;
}

inline const QStringList &ACTIONSVISIBLEONLYINEDIT()
{
    static const QStringList value = {MOVEVIEWACTION(),
                                      EXPORTVIEWTEMPLATEACTION(),
                                      REMOVEVIEWACTION()
                                     };
    return value;
}

inline const QStringList &ACTIONSSPECIAL()
{
    static const QStringList value = {SECTIONACTION(),
                                      EDITVIEWACTION()
                                     };
    return value;
}

}
}
}

#endif
