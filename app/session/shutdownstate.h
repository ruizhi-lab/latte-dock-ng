/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef LATTE_SESSION_SHUTDOWNSTATE_H
#define LATTE_SESSION_SHUTDOWNSTATE_H

namespace Latte::Session {

inline bool shouldQuitForCommittedShutdown(bool sawBlockingWindows, bool shutdownServiceActive, bool hasBlockingWindows)
{
    return (sawBlockingWindows || shutdownServiceActive) && !hasBlockingWindows;
}

}

#endif
