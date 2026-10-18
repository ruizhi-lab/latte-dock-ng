/*
    SPDX-FileCopyrightText: 2026 Latte Dock NG contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

HoverHandler {
    id: edgeHover
    blocking: false
    target: null

    required property Flickable viewport
    property bool vertical: false
    property real edgeLength: 0
    property bool canScrollBackward: false
    property bool canScrollForward: false
    property bool scrollingInProgress: false
    property int repeatInterval: 180

    signal scrollRequested(int direction)

    // Keep this as an ancestor handler, never a sibling overlay Item: the
    // latter excludes nested parabolic MouseAreas from Qt's hover traversal,
    // breaking wave feedback and wheel routing. The timer supports a stationary
    // pointer independently of task delegates entering and leaving underneath.
    readonly property int direction: {
        if (!enabled || !viewport || !viewport.visible || !viewport.enabled || !hovered) {
            return 0;
        }
        var length = vertical ? viewport.height : viewport.width;
        var position = vertical ? point.position.y : point.position.x;
        // Flickable redirects pointer handlers to its contentItem, even with
        // an explicit parent. Convert back to viewport coordinates and track
        // content offsets so stationary hover remains correct while scrolling.
        if (parent === viewport.contentItem) {
            position -= vertical ? viewport.contentY : viewport.contentX;
        }
        var trigger = Math.min(edgeLength, length / 3);
        if (position < trigger && canScrollBackward) {
            return -1;
        }
        if (position > length - trigger && canScrollForward) {
            return 1;
        }
        return 0;
    }

    readonly property Timer scrollTimer: Timer {
        interval: edgeHover.repeatInterval
        repeat: true
        running: edgeHover.direction !== 0
        onTriggered: {
            // Finish each step before scheduling another; otherwise repeated
            // targets can continually restart the existing scroll animation.
            if (!edgeHover.scrollingInProgress) {
                edgeHover.scrollRequested(edgeHover.direction);
            }
        }
    }
}
