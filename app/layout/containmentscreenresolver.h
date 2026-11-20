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

/**
 * Nested containments (a containment inside an applet, e.g. the systray) must
 * resolve the screen of the outermost containment that owns a view. Walking the
 * parent-applet chain is a policy decision independent of Plasma's ownership, so
 * it lives here as a pure step: `parentContainment()` returns the containment of
 * the parent applet, or nullptr when the walk ends.
 *
 * The depth guard stops a malformed parent graph (or a future parenting loop)
 * instead of recursing forever; `Corona::screenForContainment()` reports -1 when
 * the walk ran out of depth, which is the same answer as an unresolvable chain.
 */
class NestingContainmentResolver
{
  public:
    static constexpr int MAXDEPTH = 16;

    /**
     * Walk up from `containment` to the last containment that has a parent.
     * `parentOf` takes a containment and returns the containment owning the
     * parent applet, or nullptr when the walk ends.
     *
     * The result is the outermost containment, not the nullptr that ended the
     * walk: the caller resolves the screen of the containment a view belongs to,
     * and returning null there would drop a nested applet's dock to "unknown".
     */
    template<typename Containment, typename ParentOf>
    static Containment *outermost(Containment *containment, ParentOf &&parentOf)
    {
        Containment *outermostContainment = containment;

        for (int depth = 0; depth < MAXDEPTH && outermostContainment; ++depth) {
            Containment *parent = parentOf(outermostContainment);

            if (!parent) {
                break;
            }

            outermostContainment = parent;
        }

        return outermostContainment;
    }

    /** True when the walk stopped because the depth guard ran out. */
    template<typename Containment, typename ParentOf>
    static bool exceedsDepth(Containment *containment, ParentOf &&parentOf)
    {
        Containment *walker = containment;

        for (int depth = 0; depth < MAXDEPTH && walker; ++depth) {
            walker = parentOf(walker);
        }

        return walker != nullptr;
    }
};

} // namespace Latte::Layout
