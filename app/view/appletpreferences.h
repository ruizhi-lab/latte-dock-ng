/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef LATTE_APPLETPREFERENCES_H
#define LATTE_APPLETPREFERENCES_H

#include <QHash>
#include <QList>
#include <QSet>

namespace Latte::ViewPart {

// Clone preferences use per-containment IDs. Only mapped target entries can
// be replaced: partial discovery is not evidence that other preferences were
// disabled. Unlike applet order, an empty source is a valid preference set.
inline QList<int>
syncAppletPreferences(const QList<int> &source, const QList<int> &target, const QHash<int, int> &originalToClone, bool toClones)
{
    QHash<int, int> sourceToTarget;
    QSet<int> mappedTargets;
    for (auto it = originalToClone.constBegin(); it != originalToClone.constEnd(); ++it) {
        const int sourceId = toClones ? it.key() : it.value();
        const int targetId = toClones ? it.value() : it.key();
        sourceToTarget.insert(sourceId, targetId);
        mappedTargets.insert(targetId);
    }

    QList<int> result;
    for (const int id : source) {
        if (id < 0 || sourceToTarget.contains(id)) {
            const int translated = id < 0 ? id : sourceToTarget.value(id);
            if (!result.contains(translated)) {
                result << translated;
            }
        }
    }
    for (const int id : target) {
        if (!mappedTargets.contains(id) && !result.contains(id)) {
            result << id;
        }
    }
    return result;
}

}
#endif
