/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTETASKSTYPES_H
#define LATTETASKSTYPES_H

// Qt
#include <QObject>
#include <QMetaEnum>
#include <QMetaType>
#include <QtQml/qqmlregistration.h>

namespace Latte {
namespace Tasks {

class Types
{
    Q_GADGET
    QML_NAMED_ELEMENT(types)
    QML_UNCREATABLE("Latte Tasks Types uncreatable")

public:
    Types() = default;
    ~Types() {}

    enum Modifier {
        Shift = 0,
        Ctrl,
        Alt,
        Meta
    };
    Q_ENUM(Modifier);

    enum ClickAction {
        LeftClick = 0,
        MiddleClick,
        RightClick
    };
    Q_ENUM(ClickAction);

    enum TaskAction {
        NoneAction = 0,
        Close,
        NewInstance,
        ToggleMinimized,
        CycleThroughTasks,
        ToggleGrouping,
        PresentWindows,
        PreviewWindows,
        HighlightWindows,
        PreviewAndHighlightWindows
    };
    Q_ENUM(TaskAction);

    enum TaskScrollAction {
        ScrollNone = 0,
        ScrollTasks,
        ScrollToggleMinimized
    };
    Q_ENUM(TaskScrollAction);

    enum ManualScrollType {
        ManualScrollDisabled = 0,
        ManualScrollOnlyParallel,
        ManualScrollVerticalHorizontal
    };
    Q_ENUM(ManualScrollType);
};

}
}

#endif
