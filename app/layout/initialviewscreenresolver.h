/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

namespace Latte::Layout {

struct InitialViewScreenAssignment
{
    int runtimeScreenId{ -1 };
    int preferredScreenId{ -1 };
    bool temporaryPrimaryFallback{ false };
};

/**
 * Select the screen used to create an explicit view without discarding its
 * preferred screen when that output is temporarily absent during startup.
 */
class InitialViewScreenResolver
{
  public:
    static constexpr InitialViewScreenAssignment resolve(int preferredScreenId, bool preferredScreenActive, int primaryScreenId)
    {
        if (preferredScreenActive) {
            return { preferredScreenId, preferredScreenId, false };
        }

        return { primaryScreenId, preferredScreenId, true };
    }
};

} // namespace Latte::Layout
