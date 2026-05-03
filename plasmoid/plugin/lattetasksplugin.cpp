/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "lattetasksplugin.h"

// local
#include "contextmenuactionsbackend.h"
#include "types.h"

// Qt
#include <QtQml>
#include <QLatin1StringView>


void LatteTasksPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(QLatin1StringView(uri) == QLatin1StringView("org.kde.latte.private.tasks"));
    qmlRegisterUncreatableType<Latte::Tasks::Types>(uri, 0, 1, "types", QStringLiteral("Latte Tasks Types uncreatable"));
    qmlRegisterType<Latte::Tasks::ContextMenuActionsBackend>(uri, 0, 1, "ContextMenuActionsBackend");
}
