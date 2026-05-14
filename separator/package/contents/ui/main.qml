import QtQuick

import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

PlasmoidItem {
    id: root

    // Exposed by Latte applet communicator when this applet lives in a Latte containment.
    property QtObject latteBridge: null

    readonly property bool horizontal: plasmoid.formFactor === PlasmaCore.Types.Horizontal
    readonly property int thickness: 2
    readonly property int length: 12
    readonly property color separatorColor: {
        if (latteBridge
                && latteBridge.panelPalette
                && latteBridge.panelPalette.textColor !== undefined) {
            return latteBridge.panelPalette.textColor;
        }

        if (typeof theme !== "undefined" && theme && theme.textColor !== undefined) {
            return theme.textColor;
        }

        return "white";
    }
    readonly property real separatorOpacity: 0.4

    Plasmoid.title: i18n("Separator")
    Plasmoid.icon: "insert-horizontal-rule"
    Component {
        id: separatorRepresentation

        Item {
            implicitWidth: root.horizontal ? root.length : root.thickness
            implicitHeight: root.horizontal ? root.thickness : root.length

            Rectangle {
                anchors.centerIn: parent
                width: root.horizontal ? 1 : parent.width
                height: root.horizontal ? parent.height : 1
                radius: 1
                color: root.separatorColor
                opacity: root.separatorOpacity
            }
        }
    }

    compactRepresentation: separatorRepresentation
    fullRepresentation: separatorRepresentation
}
