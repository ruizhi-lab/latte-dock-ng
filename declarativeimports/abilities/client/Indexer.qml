/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

import org.kde.latte.abilities.definition as AbilityDefinition

AbilityDefinition.Indexer {
    id: _indexer
    property Item bridge: null
    property Item layout: null

    property bool updateIsBlocked: false

    // Incremented by the host when drag ends to force indexer bindings to
    // re-evaluate after being frozen (updateIsBlocked) during the drag.
    property int reorderToken: 0

    readonly property bool isActive: bridge !== null
    readonly property bool inMarginsArea: isActive ? bridge.indexer.inMarginsArea : false
    readonly property bool tailAppletIsSeparator: isActive ? bridge.indexer.tailAppletIsSeparator : false
    readonly property bool headAppletIsSeparator: isActive ? bridge.indexer.headAppletIsSeparator : false
    readonly property bool isReady: !updateIsBlocked
    readonly property int maxIndex: 99999
    readonly property alias firstTailItemIsSeparator: _privates.firstTailItemIsSeparator
    readonly property alias lastHeadItemIsSeparator: _privates.lastHeadItemIsSeparator
    readonly property alias itemsCount: _privates.itemsCount
    readonly property alias visibleItemsCount: _privates.visibleItemsCount
    readonly property alias firstVisibleItemIndex: _privates.firstVisibleItemIndex
    readonly property alias lastVisibleItemIndex: _privates.lastVisibleItemIndex

    QtObject {
        id: _privates
        property bool firstTailItemIsSeparator: false
        property bool lastHeadItemIsSeparator: false
        property int firstVisibleItemIndex: -1
        property int lastVisibleItemIndex: -1
        property int itemsCount: 0
        property int visibleItemsCount: 0
    }

    onIsActiveChanged: {
        if (isActive) {
            bridge.indexer.client = _indexer;
        }
    }

    Component.onCompleted: {
        if (isActive) {
            bridge.indexer.client = _indexer;
        }
    }

    Component.onDestruction: {
        if (isActive) {
            bridge.indexer.client = null;
        }
    }

    Binding {
        target: _privates
        property: "firstTailItemIsSeparator"
        when: _indexer.isReady
        value: {
            var _t = _indexer.reorderToken;
            if (_indexer.visibleItemsCount === _indexer.layout.children.length) {
                return false;
            }

            for(var i=0; i<_indexer.firstVisibleItemIndex; ++i) {
                if (_indexer.separators.indexOf(i)>=0 && _indexer.hidden.indexOf(i)<0) {
                    return true;
                }
            }

            return false;
        }
    }

    Binding {
        target: _privates
        property: "lastHeadItemIsSeparator"
        when: _indexer.isReady
        value: {
            var _t = _indexer.reorderToken;
            if (_indexer.visibleItemsCount === _indexer.layout.children.length) {
                return false;
            }

            var len = _indexer.layout.children.length;

            for(var i=len-1; i>_indexer.lastVisibleItemIndex; --i) {
                if (_indexer.separators.indexOf(i)>=0 && _indexer.hidden.indexOf(i)<0) {
                    return true;
                }
            }

            return false;
        }
    }

    Binding {
        target: _privates
        property: "firstVisibleItemIndex"
        when: _indexer.isReady
        value: {
            var _t = _indexer.reorderToken;
            var ind = _indexer.maxIndex;
            for(var i=0; i<_indexer.layout.children.length; ++i) {
                var item = _indexer.layout.children[i];
                if (item && item.itemIndex>=0
                        && !item.isSeparator /*not using "separators" array to avoid binding loops*/
                        && !item.isHidden /*not using "hidden" array to avoid binding loops*/
                        && item.itemIndex < ind) {
                    ind = item.itemIndex;
                }
            }

            return ind === _indexer.maxIndex ? -1 : ind;
        }
    }

    Binding {
        target: _privates
        property: "lastVisibleItemIndex"
        when: _indexer.isReady
        value: {
            var _t = _indexer.reorderToken;
            var ind = -1;

            for(var i=0; i<_indexer.layout.children.length; ++i) {
                var item = _indexer.layout.children[i];

                if (item && item.itemIndex>=0
                        && !item.isSeparator /*not using "separators" array to avoid binding loops*/
                        && !item.isHidden /*not using "hidden" array to avoid binding loops*/
                        && item.itemIndex > ind) {
                    ind = item.itemIndex;
                }
            }

            return ind;
        }
    }

    Binding {
        target: _privates
        property: "visibleItemsCount"
        value: {
            var count = 0;
            for(var i=0; i<_indexer.layout.children.length; ++i) {
                var item = _indexer.layout.children[i];
                if (item && item.itemIndex>=0
                        && _indexer.separators.indexOf(item.itemIndex)<0
                        && _indexer.hidden.indexOf(item.itemIndex)<0) {
                    count = count + 1;
                }
            }

            return count;
        }
    }

    Binding {
        target: _privates
        property: "itemsCount"
        value: {
            var count = 0;
            for(var i=0; i<_indexer.layout.children.length; ++i) {
                var item = _indexer.layout.children[i];
                if (item && item.itemIndex>=0) {
                    count = count + 1;
                }
            }

            return count;
        }
    }

    Binding {
        target: _indexer
        property: "hidden"
        when: _indexer.isReady
        value: {
            var _t = _indexer.reorderToken;
            var hdns = [];

            for (var i=0; i<_indexer.layout.children.length; ++i){
                var item = _indexer.layout.children[i];
                if (item && (item.isHidden || item.isSeparatorHidden) && item.itemIndex>=0 && hdns.indexOf(item.itemIndex) < 0) {
                    hdns.push(item.itemIndex);
                }
            }

            return hdns;
        }
    }

    Binding {
        target: _indexer
        property: "separators"
        when: _indexer.isReady
        value: {
            var _t = _indexer.reorderToken;
            var seps = [];

            for (var i=0; i<_indexer.layout.children.length; ++i){
                var item = _indexer.layout.children[i];
                if (item && item.isSeparator && item.itemIndex>=0 && seps.indexOf(item.itemIndex) < 0) {
                    seps.push(item.itemIndex);
                }
            }

            return seps;
        }
    }

    function visibleIndex(taskIndex) {
        if (taskIndex<firstVisibleItemIndex
                || taskIndex>lastVisibleItemIndex
                || hidden.indexOf(taskIndex) >= 0
                || separators.indexOf(taskIndex) >= 0) {
            return -1;
        }

        var vindex = -1;
        if (bridge) {
            vindex = bridge.indexer.host.visibleIndex(bridge.indexer.appletIndex);
        }

        for (var i=0; i<layout.children.length; ++i){
            var item = layout.children[i];
            if (hidden.indexOf(item.itemIndex)>=0 || separators.indexOf(item.itemIndex)>=0) {
                continue;
            }

            if (item.itemIndex<taskIndex) {
                vindex = vindex + 1;
            }
        }

        return vindex;
    }
}
