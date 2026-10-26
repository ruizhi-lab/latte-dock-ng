/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

Item {
    id: uiManager
    required property var dialog
    required property var latteView
    required property var viewConfig
    visible: false
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window

    SystemPalette {
        id: systemPalette
        colorGroup: SystemPalette.Active
    }

    readonly property color cfgBackgroundColor: (typeof dialog !== "undefined" && dialog && dialog.bC !== undefined) ? dialog.bC : systemPalette.window
    readonly property color cfgTextColor: (typeof dialog !== "undefined" && dialog && dialog.tC !== undefined) ? dialog.tC : systemPalette.windowText
    readonly property color cfgHighlightColor: (typeof dialog !== "undefined" && dialog && dialog.theme && dialog.theme.highlightColor !== undefined) ? dialog.theme.highlightColor : systemPalette.highlight
    readonly property color cfgHighlightedTextColor: (typeof dialog !== "undefined" && dialog && dialog.theme && dialog.theme.highlightedTextColor !== undefined) ? dialog.theme.highlightedTextColor : systemPalette.highlightedText

    Kirigami.Theme.backgroundColor: cfgBackgroundColor
    Kirigami.Theme.textColor: cfgTextColor
    Kirigami.Theme.highlightColor: cfgHighlightColor
    Kirigami.Theme.highlightedTextColor: cfgHighlightedTextColor


    property Item stackView: null

    Item {
        id: hiddenIndicatorPage
        anchors.fill: parent
        // Expose configuration dialog palette to dynamically loaded indicator
        // config pages via the "dialog" context property set from C++.
        property color bC: uiManager.cfgBackgroundColor
        property color tC: uiManager.cfgTextColor
        property color hC: uiManager.cfgHighlightColor
        property color htC: uiManager.cfgHighlightedTextColor
        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        Kirigami.Theme.backgroundColor: uiManager.cfgBackgroundColor
        Kirigami.Theme.textColor: uiManager.cfgTextColor
        Kirigami.Theme.highlightColor: uiManager.cfgHighlightColor
        Kirigami.Theme.highlightedTextColor: uiManager.cfgHighlightedTextColor

        readonly property int optionsWidth: dialog.optionsWidth
        readonly property bool deprecatedOptionsAreHidden: true // @since 0.10.0

        //! Shows the options of the selected indicator. C++
        //! IndicatorUiManager::ui() only re-parents the type's cached options
        //! item into this item, even when it is already shown, and provides
        //! nothing for indicators without X-Latte-ConfigUi. This QML owns where
        //! the item goes, so every request has to end in showNextIndicator():
        //! skipping it for an indicator without options kept the previous
        //! indicator's options on screen.
        function showCurrentIndicator() {
            var type = latteView.indicator.type;
            tabBar.selectTab(type);
            viewConfig.indicatorUiManager.ui(type, latteView);
            showNextIndicator(type);
        }

        //! Decide by the shown page's type, not by the pages' isCurrent
        //! binding: ui() has just moved the options out of the shown page even
        //! when they belong to the requested type, and isCurrent (whose
        //! viewConfig.isReady term re-evaluates in signal connection order)
        //! then skipped the update, parked the options in hiddenPages and left
        //! the options area blank for good.
        function showNextIndicator(type) {
            var nextIndicator = children.length > 0 ? children[0] : null;
            var currentPage = stackView.currentItem;

            if (currentPage && currentPage.type === type) {
                //! keep the shown page and put its options back
                if (nextIndicator) {
                    nextIndicator.parent = currentPage;
                    nextIndicator.visible = true;
                    stackView.Layout.minimumHeight = nextIndicator.height;
                }
            } else {
                var currentIndex = -1;

                if (currentPage) {
                    currentIndex = viewConfig.indicatorUiManager.index(currentPage.type);
                }

                var nextIndex = viewConfig.indicatorUiManager.index(type);

                stackView.forwardSliding = (nextIndex<currentIndex);
                stackView.Layout.minimumHeight = nextIndicator ? nextIndicator.height : 0;

                //! slide in a new page holding the options, or nothing for an
                //! indicator without options
                var page = stackView.replace(currentPage, indicatorPageComponent, {"type": type});

                if (nextIndicator) {
                    nextIndicator.parent = page;
                    nextIndicator.visible = true;
                }
            }

            if (children.length>0) {
                //!empty all remaining pages by moving them into hiddenPages
                var tempcount = children.length;
                for (var i=0; i<tempcount; ++i) {
                    var tempIndicator = children[0];
                    tempIndicator.visible = false;
                    tempIndicator.parent = hiddenPages;
                }
            }
        }

        Component.onCompleted: {
            viewConfig.indicatorUiManager.setParentItem(hiddenIndicatorPage);
            showCurrentIndicator();
        }

        Connections {
            target: latteView.indicator
            function onPluginChanged() {
                if (viewConfig.isReady) {
                    hiddenIndicatorPage.showCurrentIndicator();
                }
            }
        }

        Connections {
            target: viewConfig
            function onIsReadyChanged() {
                if (viewConfig.isReady) {
                    hiddenIndicatorPage.showCurrentIndicator();
                }
            }
        }
    }

    //! Every switch shows a new page, which StackView owns and deletes after it
    //! slid out. Two reused pages kept the end state of their replaceExit
    //! transition (opacity 0) and relied on replaceEnter to undo it, but Qt
    //! skips replaceEnter while the view has no height, which is the case
    //! right after an indicator without options collapsed the area: the
    //! options came back fully transparent. A page reused while it was still
    //! sliding out could also be faded out again by its exit animation.
    Component {
        id: indicatorPageComponent

        ColumnLayout {
            id: indicatorPage
            width: stackView.width
            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.Window
            Kirigami.Theme.backgroundColor: uiManager.cfgBackgroundColor
            Kirigami.Theme.textColor: uiManager.cfgTextColor
            Kirigami.Theme.highlightColor: uiManager.cfgHighlightColor
            Kirigami.Theme.highlightedTextColor: uiManager.cfgHighlightedTextColor

            readonly property bool isCurrent: latteView.indicator.type === type && viewConfig.isReady/*update flag*/
            readonly property bool deprecatedOptionsAreHidden: true // @since 0.10.0
            readonly property int optionsWidth: dialog.optionsWidth

            property string type: ""

            //! the cached options outlive their page: park them before
            //! StackView deletes it instead of leaving them without a parent
            QQC2.StackView.onRemoved: {
                var count = indicatorPage.children.length;
                for (var i=0; i<count; ++i) {
                    var options = indicatorPage.children[0];
                    options.visible = false;
                    options.parent = hiddenPages;
                }
            }
        }
    }

    Item {
        id: hidden
        visible: false
        ColumnLayout {
            id: hiddenPages
            width: stackView.width
            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.Window
            Kirigami.Theme.backgroundColor: uiManager.cfgBackgroundColor
            Kirigami.Theme.textColor: uiManager.cfgTextColor
            Kirigami.Theme.highlightColor: uiManager.cfgHighlightColor
            Kirigami.Theme.highlightedTextColor: uiManager.cfgHighlightedTextColor

            readonly property bool isCurrent: latteView.indicator.type === type && viewConfig.isReady/*update flag*/
            readonly property bool deprecatedOptionsAreHidden: true // @since 0.10.0
            readonly property int optionsWidth: dialog.optionsWidth

            property string type: ""
        }
    }
}
