/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.7
import QtQuick.Window 2.2
import QtQuick.Templates 2.2 as T
import QtQuick.Controls 2.2 as Controls
import QtQuick.Layouts 1.3
import QtQuick.Effects
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kirigami 2.2 as Kirigami
import org.kde.ksvg 1.0 as KSvg
import "private" as Private

T.ComboBox {
    id: control
    Kirigami.Theme.inherit: true

    readonly property var theme: Kirigami.Theme
    readonly property var units: Kirigami.Units
    readonly property color safeTextColor: (theme && theme.textColor !== undefined) ? theme.textColor : "white"
    readonly property color safeHighlightedTextColor: (theme && theme.highlightedTextColor !== undefined) ? theme.highlightedTextColor : safeTextColor
    readonly property color safeButtonTextColor: safeTextColor
    readonly property color safeBackgroundColor: (theme && theme.backgroundColor !== undefined) ? theme.backgroundColor : "white"
    readonly property color safeButtonColor: safeBackgroundColor
    readonly property color safeButtonBorderColor: Qt.rgba(safeTextColor.r, safeTextColor.g, safeTextColor.b, 0.35)
    readonly property color safePressedBorderColor: (theme && theme.highlightColor !== undefined)
                                                    ? theme.highlightColor
                                                    : safeButtonBorderColor
    readonly property int frameMargin: units.smallSpacing

    function modelItemAt(idx) {
        if (!model || idx < 0) {
            return null;
        }

        if (Array.isArray(model)) {
            return model[idx];
        }

        if (typeof model.get === "function") {
            return model.get(idx);
        }

        return model[idx];
    }

    function modelRoleAt(idx, role) {
        if (!role || role.length === 0) {
            return "";
        }

        var item = modelItemAt(idx);
        return item ? item[role] : "";
    }

    implicitWidth: Math.max(background ? background.implicitWidth : 0,
                            contentItem.implicitWidth + leftPadding + rightPadding) + indicator.implicitWidth + rightPadding
    implicitHeight: units.gridUnit * 1.6
    baselineOffset: contentItem.y + contentItem.baselineOffset

    hoverEnabled: true
    topPadding: frameMargin
    leftPadding: frameMargin
    rightPadding: frameMargin + units.gridUnit * 2
    bottomPadding: frameMargin

    wheelEnabled: false

    property bool hideDisplayText: false
    property bool hideSelectedItemIcon: false

    property bool blankSpaceForEmptyIcons: false
    property bool forcePressed: false
    property bool popUpAlignRight: true
    property bool buttonIsTransparent: false
    property int minimumPopUpWidth: 150
    property int popUpRelativeX: 0
    property int popUpTextHorizontalAlignment: Text.AlignLeft

    property string enabledRole
    property string iconRole
    property string iconToolTipRole
    property string iconOnlyWhenHoveredRole
    property string isSeparatorRole
    property string toolTipRole

    signal iconClicked(int index);

    delegate: ItemDelegate {
        width: control.popup.width
        enabled: !isSeparator && (control.enabledRole.length>0 ? (isArray ? modelData[control.enabledRole] : model[control.enabledRole]) : true)
        text: control.textRole.length>0 ? (isArray ? modelData[control.textRole] : model[control.textRole]) : modelData
        iconName: control.iconRole.length>0 ? (isArray ? modelData[control.iconRole] : model[control.iconRole]) : ''
        iconToolTip: control.iconToolTipRole.length>0 ? (isArray ? modelData[control.iconToolTipRole] : model[control.iconToolTipRole]) : ''
        iconOnlyWhenHovered: control.iconOnlyWhenHoveredRole.length>0 ? (isArray ? modelData[control.iconOnlyWhenHoveredRole] : model[control.iconOnlyWhenHoveredRole]) : ''
        isSeparator: control.isSeparatorRole.length>0 ? (isArray ? modelData[control.isSeparatorRole] : model[control.isSeparatorRole]) : false
        toolTip: control.toolTipRole.length>0 ? (isArray ? modelData[control.toolTipRole] : model[control.toolTipRole]) : ''

        highlighted: mouseArea.pressed ? listView.currentIndex == index : control.currentIndex == index
        blankSpaceForEmptyIcons: control.blankSpaceForEmptyIcons
        textHorizontalAlignment: popUpTextHorizontalAlignment

        readonly property bool isArray: Array.isArray(control.model)
        property bool separatorVisible: false

        PlasmaComponents.Button {
            id: tooltipBtn
            anchors.fill: parent
            opacity: 0
            visible: parent.toolTip !== ''

            PlasmaComponents.ToolTip { text: tooltipBtn.parent ? tooltipBtn.parent.toolTip : "" }

            onPressedChanged: {
                if (!pressed) {
                    control.currentIndex = index;
                    control.down = false;
                    control.popup.visible = false;
                }
            }
        }
    }

    indicator: KSvg.SvgItem {
        implicitWidth: units.iconSizes.small
        implicitHeight: implicitWidth
        anchors {
            right: parent.right
            rightMargin: control.buttonIsTransparent ? 0 : frameMargin
            verticalCenter: parent.verticalCenter
        }
        svg: KSvg.Svg {
            imagePath: "widgets/arrows"
        }
        elementId: "down-arrow"
    }

    //     contentItem: Label {
    //         text: control.displayText
    //         font: control.font
    //         color: theme.buttonTextColor
    //         horizontalAlignment: Text.AlignLeft
    //         verticalAlignment: Text.AlignVCenter
    //         elide: Text.ElideRight
    //     }
    contentItem: MouseArea {
        id: mouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        preventStealing: true
        property int indexUnderMouse: -1
        onWheel: {
            if (!control.wheelEnabled) {
                return;
            }

            if (wheel.pixelDelta.y < 0 || wheel.angleDelta.y < 0) {
                control.currentIndex = Math.min(control.currentIndex + 1, delegateModel.count -1);
            } else {
                control.currentIndex = Math.max(control.currentIndex - 1, 0);
            }
            control.activated(control.currentIndex);
        }
        onPressed: onGenericPressed()
        onReleased: onGenericReleased()
        onCanceled: {
            control.down = false;
        }
        onPositionChanged: {
            var pos = listView.mapFromItem(this, mouse.x, mouse.y);
            indexUnderMouse = listView.indexAt(pos.x, pos.y);
            listView.currentIndex = indexUnderMouse;
            control.activated(indexUnderMouse);
        }

        Connections {
            target: popup
            function onClosed() {
                control.down = false;
            }
        }

        function onGenericPressed() {
            indexUnderMouse = -1;
            listView.currentIndex = control.highlightedIndex;
            control.down = true;
            control.popup.visible = !control.popup.visible;
        }

        function onGenericReleased() {
            if (!containsMouse && !hiddenTooltipButton.hovered) {
                control.down = false;
                control.popup.visible = false;
            }
            if (indexUnderMouse > -1) {
                control.currentIndex = indexUnderMouse;
            }
        }

        PlasmaComponents.Button {
            id: hiddenTooltipButton
            anchors.fill: parent
            opacity: 0
            visible: control && control.currentIndex>=0 && control.toolTipRole.length>0

            property string computedTooltip: {
                if (!visible) return "";
                return control.modelRoleAt(control.currentIndex, control.toolTipRole);
            }

            PlasmaComponents.ToolTip { text: hiddenTooltipButton.computedTooltip }

            onPressedChanged: {
                if (pressed) {
                    mouseArea.onGenericPressed();
                } else {
                    mouseArea.onGenericReleased();
                }
            }
        }

        RowLayout {
            anchors.fill: parent
            spacing: 0

            anchors {
                leftMargin: !control.mirrored ? 1 : 0
                rightMargin: control.mirrored ? 1 : 0
            }

            Kirigami.Icon {
                id: selectedIcon
                implicitWidth: textLabel.height
                implicitHeight: textLabel.height

                source: {
                    if (control
                            && control.currentIndex>=0
                            && control.iconRole.length>0) {
                        return control.modelRoleAt(control.currentIndex, control.iconRole);
                    }

                    return "";
                }

                visible: source !== '' && !control.hideSelectedItemIcon
            }

            Label {
                id: textLabel
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: !selectedIcon.visible && !control.mirrored ? units.smallSpacing : 0
                Layout.rightMargin: !selectedIcon.visible && control.mirrored ? units.smallSpacing : 0

                text: control.displayText
                font: control.font
                color: control.pressed
                       ? safeHighlightedTextColor
                       : (control.buttonIsTransparent ? safeTextColor : safeButtonTextColor)
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                opacity: control.enabled ? 1 : 0.6

                visible: !control.hideDisplayText
            }

         /*   T.TextField {
                id: textField
                padding: 0
                Layout.fillWidth: true
                Layout.fillHeight: true

                text: control.editable ? control.editText : control.displayText

                enabled: control.editable
                autoScroll: control.editable

                readOnly: control.down || !control.hasOwnProperty("editable") || !control.editable
                inputMethodHints: control.inputMethodHints
                validator: control.validator

                // Work around Qt bug where NativeRendering breaks for non-integer scale factors
                // https://bugreports.qt.io/browse/QTBUG-67007
                renderType: Screen.devicePixelRatio % 1 !== 0 ? Text.QtRendering : Text.NativeRendering
                color: theme.buttonTextColor //control.enabled ? theme.textColor : theme.disabledTextColor
                selectionColor: Kirigami.Theme.highlightColor
                selectedTextColor: Kirigami.Theme.highlightedTextColor

                selectByMouse: !Kirigami.Settings.tabletMode
                cursorDelegate: Kirigami.Settings.tabletMode ? mobileCursor : undefinedCursor

                font: control.font
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                opacity: control.enabled ? 1 : 0.6
                onFocusChanged: {
                    if (focus) {
                        Private.MobileTextActionsToolBar.controlRoot = textField;
                    }
                }

                onPressAndHold: {
                    if (!Kirigami.Settings.tabletMode) {
                        return;
                    }
                    forceActiveFocus();
                    cursorPosition = positionAt(event.x, event.y);
                    selectWord();
                }
            }*/
        }
    }

  /*  Component {
        id: mobileCursor
        Private.MobileCursor {
            target: textField
        }
    }*/

    Component {
        id: undefinedCursor
        Item{}
    }

 /*   Private.MobileCursor {
        target: textField
        selectionStartHandle: true
        property var rect: textField.positionToRectangle(textField.selectionStart)
        //FIXME: this magic values seem to be always valid, for every font,every dpi, every scaling
        x: rect.x + 5
        y: rect.y + 6
    }*/

    background: Rectangle {
        id: surfaceNormal
        implicitWidth: units.gridUnit * 6
        width: parent.width
        height: parent.height
        radius: Math.max(2, units.smallSpacing)
        color: safeButtonColor
        border.width: 1
        border.color: control.pressed || control.forcePressed
                      ? safePressedBorderColor
                      : safeButtonBorderColor

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: control.buttonIsTransparent ? -frameMargin : 0
        opacity: control.buttonIsTransparent && !control.pressed && !control.popup.visible ? 0 : 1

        MouseArea {
            anchors {
                fill: parent
                leftMargin: control.leftPadding
                rightMargin: control.rightPadding
            }
            acceptedButtons: Qt.NoButton
            onWheel: {
                if (!control.wheelEnabled) {
                    return;
                }

                if (wheel.pixelDelta.y < 0 || wheel.angleDelta.y < 0) {
                    control.currentIndex = Math.min(control.currentIndex + 1, delegateModel.count -1);
                } else {
                    control.currentIndex = Math.max(control.currentIndex - 1, 0);
                }
                control.activated(control.currentIndex);
            }
        }
    }

    popup: T.Popup {
        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.Window

        x: {
            if (!control.mirrored) {
                if (popUpRelativeX !== 0) {
                    var adjustedX = exceedsContent && control.popUpAlignRight ? -(width - control.width) : popUpRelativeX;
                    return  adjustedX;
                } else {
                    return 0;
                }
            } else {
                //! mirrored case
                if (exceedsContent && control.popUpAlignRight) {
                    var adjustedX = width - control.width - popUpRelativeX;
                    return  -adjustedX;
                } else {
                    return 0;
                }
            }
        }
        y: control.height
        width: Math.max(control.width, control.minimumPopUpWidth)
        implicitHeight: contentItem.implicitHeight
        topMargin: 6
        bottomMargin: 6

        readonly property bool exceedsContent: control.width < width

        /*onVisibleChanged: {
            if (visible) {
                console.log("  mirrored:" + control.mirrored);
                console.log("  exceeds: " + exceedsContent);
                console.log("  popupAR: " + control.popUpAlignRight);
                console.log("  popupRX: " + popUpRelativeX);
            }
        }*/

        contentItem: ListView {
            id: listView
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            highlightRangeMode: ListView.ApplyRange
            highlightMoveDuration: 0
            // HACK: When the ComboBox is not inside a top-level Window, it's Popup does not inherit
            // the LayoutMirroring options. This is a workaround to fix this by enforcing
            // the LayoutMirroring options properly.
            // QTBUG: https://bugreports.qt.io/browse/QTBUG-66446
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true
            T.ScrollBar.vertical: Controls.ScrollBar { }

            signal iconClicked(int index);

            onIconClicked: control.iconClicked(index);
        }
        background: Rectangle {
            anchors {
                fill: parent
                margins: -1
            }
            radius: 2
            color: safeBackgroundColor
            border.color: Qt.rgba(safeTextColor.r, safeTextColor.g, safeTextColor.b, 0.3)
            layer.enabled: true

            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: Qt.rgba(0, 0, 0, 0.3)
                shadowBlur: 0.125
                shadowHorizontalOffset: 2
                shadowVerticalOffset: 2
            }
        }
    }
}
