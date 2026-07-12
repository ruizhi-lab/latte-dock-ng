/*
    SPDX-FileCopyrightText: 2026 Latte Dock Contributors
    SPDX-License-Identifier: GPL-2.0-or-later

    Minimal PipeWire warmup — instantiates PipeWire.PipeWireSourceItem
    without a valid nodeId just to trigger pw_init() and connect to the
    PipeWire daemon so that subsequent PipeWireSourceItem creations (in
    ToolTipInstance) are fast.
*/

import QtQuick
import org.kde.pipewire as PipeWire

PipeWire.PipeWireSourceItem {
    // nodeId stays 0 (no stream) — we only need the PipeWire
    // infrastructure initialised.
}
