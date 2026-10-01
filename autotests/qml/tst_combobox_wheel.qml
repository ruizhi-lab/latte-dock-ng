import QtQuick
import QtTest
import "../../declarativeimports/components" as LatteComponents

TestCase {
    id: testCase
    name: "LatteComboBoxWheel"
    when: windowShown
    visible: true
    width: 340
    height: 140

    property int activationCount: 0

    function init() {
        combo.currentIndex = 1
        combo.wheelEnabled = true
        activationCount = 0
    }

    LatteComponents.ComboBox {
        id: combo
        width: 260
        model: ["first", "second", "third"]
        currentIndex: 1
        wheelEnabled: true
        onActivated: function(index) { testCase.activationCount++ }
    }

    function test_selectionAndBounds() {
        waitForRendering(combo)
        mouseWheel(combo, 30, 15, 0, -120)
        compare(combo.currentIndex, 2)
        compare(activationCount, 1)

        mouseWheel(combo, 30, 15, 0, -120)
        compare(combo.currentIndex, 2)
        compare(activationCount, 2)

        mouseWheel(combo, 30, 15, 0, 120)
        compare(combo.currentIndex, 1)
        compare(activationCount, 3)
    }

    function test_wheelOnTransparentBackgroundRegion() {
        waitForRendering(combo)
        mouseWheel(combo, 250, 15, 0, -120)
        compare(combo.currentIndex, 2)
        compare(activationCount, 1)

        combo.wheelEnabled = false
        mouseWheel(combo, 250, 15, 0, 120)
        compare(combo.currentIndex, 2)
        compare(activationCount, 1)
    }
}
