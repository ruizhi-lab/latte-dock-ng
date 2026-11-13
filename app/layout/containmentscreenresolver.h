/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <optional>

namespace Latte::Layout {

struct PendingContainmentScreen
{
    bool onPrimary{ false };
    int screen{ -1 };
};

/**
 * Resolve a containment screen while keeping runtime state reads lazy.
 * Pending updates are authoritative; callbacks for live views and fallback
 * must not run until the preceding source has no answer. An optional view ID
 * distinguishes no mapping from ScreenPool's negative unknown/disconnected
 * sentinel; the sentinel remains authoritative at this layer.
 */
class ContainmentScreenResolver
{
  public:
    template<typename PrimaryScreenId, typename ViewScreenId, typename LastScreen>
    static int resolve(const std::optional<PendingContainmentScreen> &pending, bool hasCorona, PrimaryScreenId &&primaryScreenId, ViewScreenId &&viewScreenId, LastScreen &&lastScreen)
    {
        if (pending) {
            return pending->onPrimary && hasCorona ? primaryScreenId() : pending->screen;
        }

        if (const auto screenId = viewScreenId()) {
            return *screenId;
        }

        return lastScreen();
    }
};

} // namespace Latte::Layout
