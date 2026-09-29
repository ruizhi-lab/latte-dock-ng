/*
    SPDX-FileCopyrightText: 2026 Ruizhi Zhong <ruizhi.zhong88@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef BLOCKHIDINGEVENTS_H
#define BLOCKHIDINGEVENTS_H

#include <QStringList>

namespace Latte::ViewPart {

// VisibilityManager owns this set as the authoritative list of active hide
// blockers. Reporting only blocked/unblocked transitions keeps overlapping
// menu, drag and edit blockers from hiding the view when one owner exits.
class BlockHidingEvents
{
public:
    bool isBlocked() const noexcept
    {
        return !m_events.isEmpty();
    }

    bool hasEvent(const QString &event) const
    {
        return !event.isEmpty() && m_events.contains(event);
    }

    bool addEvent(const QString &event)
    {
        if (event.isEmpty() || m_events.contains(event)) {
            return false;
        }

        const bool wasBlocked = isBlocked();
        m_events.append(event);
        return wasBlocked != isBlocked();
    }

    bool removeEvent(const QString &event)
    {
        if (event.isEmpty() || !m_events.contains(event)) {
            return false;
        }

        const bool wasBlocked = isBlocked();
        m_events.removeAll(event);
        return wasBlocked != isBlocked();
    }

    qsizetype count() const noexcept
    {
        return m_events.size();
    }

private:
    QStringList m_events;
};

}

#endif
